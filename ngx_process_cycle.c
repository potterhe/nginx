#include <unistd.h>
#include <signal.h>
#include "ngx_process.h"
#include "ngx_process_cycle.h"

/**
 * 标识当前进程的角色，master, worker, ...
 * 在信号处理时，需要区分，因为master,worker等都共享ngx_signal_handler()这个信号处理函数
 */
unsigned int ngx_process;

int ngx_reap;//回收子进程
int ngx_reconfigure;
int ngx_terminate;
int ngx_quit;

extern ngx_process_t ngx_processes[NGX_MAX_PROCESSES];

static void ngx_worker_process_cycle();
static void ngx_worker_process_init();
static int ngx_worker_process_exit();
static void ngx_signal_worker_processes(int signo);
static int ngx_reap_children();

void
ngx_master_process_cycle()
{
    sigset_t set;
    /*
     * block a lot signals
     */
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGHUP); 
    sigaddset(&set, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
    
    }
    sigemptyset(&set);

    /* 在nginx的源代码中没能找到初始化全局数组 ngx_processes 的代码
     * 这里是一个合适的位置
     */
    ngx_init_processes_array();

    ngx_start_worker_processes(2);

    /*
     * watch worker process
     */
    for ( ; ; ) {
    
	/* 
	 * master process will be blocked here,
	 * waiting for a signal to wake up
	 */
	sigsuspend(&set);

	/**
	 * 回收子进程,这里的回收是针对全局数组 ngx_processes, 
	 * 操作系统层面的进程回收已经在信号处理函数 ngx_signal_handler() 中完成, 
	 * 并对ngx_processes 的相应条目中进行了标记
	 */
	if (ngx_reap) {
	    ngx_reap = 0;
	    ngx_reap_children();
	}

	/* 
	 * here, ngx_signal_handler() had returned
	 * ngx_signal_handler() 会根据接收到的信号，设置几个全局变量的值
	 * 我们需要在这里检测这几全局值，正确响应信号约定的行为
	 *
	 * ngx_terminate "nginx -s stop" or "kill -s SIGTERM MASTER_PID"
	 *
	 * for this option, nginx had optimized. 
	 * first, master send command to worker process by IPC socket.
	 * if worker process did not quit in 1000 ms, master send SIGKILL signal.
	 */
	if (ngx_terminate) {
	    ngx_signal_worker_processes(SIGTERM);
	    continue;
	}

	/* 
	 * ngx_reconfigure "nginx -s reload" or "kill -s SIGHUP MASTER_PID"
	 *
	 * 在实现重载配置文件功能时,nginx首先孵化新的worker进程,新进程读取新重的配置文件
	 * 然后向旧的worker进程发送SIGQUIT command,通过IPC socket
	 */
	if (ngx_reconfigure) {
	    ngx_reconfigure = 0;

	    /*TODO ngx_start_worker_processes()*/
	    ngx_signal_worker_processes(SIGQUIT);
	}

    }
}

void
ngx_start_worker_processes(int n)
{
    int i;

    for (i = 0; i < n; i++) {
	ngx_spawn_process(ngx_worker_process_cycle);
    }
}

static void
ngx_worker_process_cycle()
{
    //sign self is a worker process
    ngx_process = NGX_PROCESS_WORKER;

    ngx_worker_process_init();

    for ( ; ; ) {
	ngx_process_events_and_timers();

	if (ngx_terminate) {
	    ngx_worker_process_exit();
	}
    }
}

static void
ngx_worker_process_init()
{
    sigset_t set;
    int n;

    /*
    ngx_set_environment();
    setpriority();
    setrlimit(RLIMIT_NOFILE);
    setrlimit(RLIMIT_CORE);
    setrlimit(RLIMIT_SIGPENDING);
    setgid
    initgroups
    setuid
    sched_setaffinity
    chdir
    */

    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
    
    }

    /* module init*/
    //ngx_modules[i]->init_process();

    /* 关闭全局数组 ngx_processes 中 master 持有的其它进程的ipc socket [0]
     * 在 ngx_spawn_process() 中我们提到,nginx没有关闭 fd[1]
     * 这里的实现,worker进程是关闭master持有的其它进程的 fd[1]，但没有关闭fd[0]
     * 按理说，worker进程应该关闭掉其它进程的fd[0]才对，fd[0]是master发送cmd的端口.
     * 在worker进程中，全局数组ngx_processes应该仅自身的fd[1]打开，其它全关闭.
     */
    for (n = 0; n < NGX_MAX_PROCESSES; n++) {
	if (ngx_processes[n].pid == -1) {
	    continue;
	}

	if (ngx_processes[n].ipcfd == -1) {
	    continue;
	}

	close(ngx_processes[n].ipcfd);
	ngx_processes[n].ipcfd = -1;
    }

    //ngx_add_channel_event
}

static int
ngx_worker_process_exit()
{
    ngx_log_stderr("worker exit\n");
    exit(0);
}

static void
ngx_signal_worker_processes(int signo)
{
    int i, command, len;
    char c[10];
    /**
     * NGX_SHUTDOWN_SIGNAL QUIT
     * NGX_TERMINATE_SIGNAL TERM, 
     * NGX_REOPEN_SIGNAL    USR1
     * above signo, IPC socket
     */
    switch (signo) {
	case SIGQUIT:
	    command = NGX_CMD_QUIT;
	    break;

	case SIGTERM:
	    command = NGX_CMD_TERMINATE;
	    break;

	case SIGUSR1:
	    command = 1;
	    break;

	default:
	    command = 0;
    }

    for (i = 0; i < NGX_MAX_PROCESSES; i++) {
	if (ngx_processes[i].pid == -1)
	    continue;

	/* ipc socket*/
	if (command) {
	    len = snprintf(c, 10, "%d", command);
	    printf("write command: len = %d\n", len);
	    write(ngx_processes[i].ipcfd, c, len);
	    continue;
	}

	kill(ngx_processes[i].pid, signo);
    }
}

static int
ngx_reap_children()
{
    /* TODO 清理全局数组 ngx_processes 
     * 关闭 master 持有的已结束进程的 ipc socket
     */
    return 0;
}
