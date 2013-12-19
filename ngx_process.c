#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <string.h>
#include "ngx_process.h"
#include "ngx_log.h"

typedef struct {
    int signo;
    char *name; //对应命令行参数-s语义,不适用-s的为空串
    void (*handler) (int signo);
} ngx_signal_t;

ngx_process_t    ngx_processes[NGX_MAX_PROCESSES];
int worker_ipcfd; //worker process use only

extern int ngx_reconfigure;
extern int ngx_terminate;
extern int ngx_reap;
extern int ngx_sigalrm;

static void ngx_process_get_status();
void ngx_signal_handler(int signo);

ngx_signal_t signals[] = {
    {SIGTERM, "stop", ngx_signal_handler},
    {SIGHUP, "reload", ngx_signal_handler},
    {SIGCHLD, "", ngx_signal_handler},
    {SIGALRM, "", ngx_signal_handler},
    {0, NULL, NULL}
};

void
ngx_init_processes_array()
{
    int i;
    for (i = 0; i < NGX_MAX_PROCESSES; i++) {
	ngx_processes[i].pid = -1;
	ngx_processes[i].ipcfd = -1;
    }
}

int
ngx_init_signals()
{
    ngx_signal_t *s;
    struct sigaction sa;

    for (s = signals; s->signo != 0; s++) {
		//初始化sa内存,如果使用这个，则下面给sa_flags赋值就不必要了
		memset(&sa, 0, sizeof(struct sigaction));
        sa.sa_handler = s->handler;
        sigemptyset(&sa.sa_mask);
		//sa.sa_flags = 0;

		if (sigaction(s->signo, &sa, NULL) == -1) {
			ngx_log_error("sigaction(%d) failed", s->signo);
			return SIG_ERR;
		}
    }
    return 0;
}

void
ngx_signal_handler(int signo)
{
    ngx_signal_t *s;

    ngx_log_error("signo:%d", signo);
    for (s = signals; s->signo != 0; s++) {
		if (s->signo == signo) {
			break;
		}
    }

    switch (signo) {

	case SIGALRM:
	    ngx_sigalrm = 1;
	    break;
	
	case SIGCHLD:
	    ngx_reap = 1;
	    break;

	case SIGHUP:
	    ngx_reconfigure = 1;
	    break;

	case SIGTERM:
	    ngx_terminate = 1;
	    break;

    }

    if (signo == SIGCHLD) {
		ngx_process_get_status();
    }
}

static void
ngx_process_get_status()
{
    int status, i;
    pid_t pid;

    for ( ; ; ) {
	pid = waitpid(-1, &status, WNOHANG);

	if (pid == 0) {
	    return;
	}

	if (pid == -1) {
	    /* TODO 错误处理，日志 */
	    return;
	}

	/* 标记全局数组 ngx_processes 该进程的退出状态
	 * 具体清理的细节在 ngx_reap_children() 中实现
	 */
	for (i = 0; i < NGX_MAX_PROCESSES; i++) {
	    if (ngx_processes[i].pid == pid) {
		//ngx_processes[i].status = status;
                ngx_processes[i].exited = 1;
		break;
	    }
	}

	/* TODO 通过WIF宏分析 status,确定进程退出状态，记录进日志 */
	ngx_log_error("process exited %d", pid);
    }
}

pid_t
ngx_spawn_process(ngx_spawn_proc_pt proc)
{
    pid_t pid;
    int fd[2], s;

    for (s = 0; s < NGX_MAX_PROCESSES; s++) {
	if (ngx_processes[s].pid == -1)
	    break;
    }

    /*
     * IPC socket
     */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == -1) {
	return -1;
    }
    /*TODO 
     * nonblocking
     * [0], FIOASYNC
     * [0], F_SETOWN
     * FD_CLOEXEC
     */

    pid = fork();

    switch(pid) {
	case -1:
	    close(fd[0]);
	    close(fd[1]);
	    return -1;

	case 0:	
	    close(fd[0]);
	    worker_ipcfd = fd[1];
	    proc();//ngx_worker_process_cycle();
	    break;

	default:
	    break;
    }

    /* master 虽然只使用fd[0], 但它没有关闭fd[1]
     * 在源代码中没有找到类似 ngx_processes[i].channel[1] 的引用
     * 这里的关闭操作是"我"添加的 
     */
    close(fd[1]);

    ngx_processes[s].pid = pid;
    ngx_processes[s].ipcfd = fd[0];

    ngx_log_error("master: worker process spawn %d", pid);

    return pid;
}
