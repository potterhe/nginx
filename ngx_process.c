#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include "ngx_process.h"

ngx_process_t    ngx_processes[NGX_MAX_PROCESSES];
int worker_ipcfd;
extern ngx_reconfigure;

void ngx_signal_handler(int signo);

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
    int signo = SIGHUP;
    struct sigaction sa;

    sa.sa_handler = ngx_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(signo, &sa, NULL) == -1)
	return SIG_ERR;

    return 0;
}

void
ngx_signal_handler(int signo)
{
    switch (signo) {
    case SIGHUP:
	ngx_reconfigure = 1;
	break;
    }
}

pid_t
ngx_spawn_process(ngx_spawn_proc_pt proc)
{
    pid_t pid;
    int fd[2], i;

    for (i = 0; i < NGX_MAX_PROCESSES; i++) {
	if (ngx_processes[i].pid == -1)
	    break;
    }

    /*
     * IPC socket
     */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == -1) {
	return -1;
    }

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

    ngx_processes[i].pid = pid;
    ngx_processes[i].ipcfd = fd[0];

    return pid;
}
