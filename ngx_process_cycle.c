#include <signal.h>
#include "ngx_process.h"

int ngx_reconfigure;
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
	/*TODO ngx_signal_handler() had returned */
	ngx_signal_worker_processes(SIGHUP);
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
    char c[] = "abc";
    /**
     * NGX_SHUTDOWN_SIGNAL QUIT
     * NGX_TERMINATE_SIGNAL TERM, 
     * NGX_REOPEN_SIGNAL    USR1
     * above signo, IPC socket
     */
    switch (signo) {
	case SIGQUIT:
	case SIGTERM:
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
	    write(ngx_processes[i].ipcfd, c, sizeof(c));
	    continue;
	}

	kill(ngx_processes[i].pid, signo);
    }
}
