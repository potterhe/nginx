#include <unistd.h>
#include <signal.h>
#include "ngx_process.h"
#include "ngx_process_cycle.h"

int ngx_reconfigure;
int ngx_terminate;

extern ngx_process_t ngx_processes[NGX_MAX_PROCESSES];

static void ngx_worker_process_cycle();
static void ngx_worker_process_init();
static void ngx_signal_worker_processes(int signo);

void
ngx_master_process_cycle()
{
    sigset_t set;
    /*
     * block a lot signals
     */
    sigemptyset(&set);
    sigaddset(&set, SIGHUP); 

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
    ngx_worker_process_init();

    for ( ; ; ) {
	ngx_process_events_and_timers();
    }
}

static void
ngx_worker_process_init()
{
    sigset_t set;

    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
    
    }

    /* module init*/
    //ngx_modules[i]->init_process();
}

static void
ngx_signal_worker_processes(int signo)
{
    int i, command;
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
	    snprintf(c, 10, "%d", command);
	    write(ngx_processes[i].ipcfd, c, sizeof(c));
	    continue;
	}

	kill(ngx_processes[i].pid, signo);
    }
}
