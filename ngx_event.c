#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>//for atoi()
#include "ngx_process_cycle.h"

extern int worker_ipcfd;
extern int ngx_quit;
extern int ngx_terminate;

void
ngx_process_events_and_timers()
{
    char b[1024];
    ssize_t n;
    int command;

    /*worker process will be blocked here, waiting for master's signal from ipc socket*/
    if ((n = read(worker_ipcfd, b, 1024)) > 0) {
	//printf("worker process %d revieve msg: %s, len %d\n", getpid(), b, n);

	b[n] = '\0';
	command = atoi(b);
	switch (command) {

	    case NGX_CMD_QUIT:
		ngx_quit = 1;
		break;

	    case NGX_CMD_TERMINATE:
		ngx_terminate = 1;
		break;
	
	}
    }
}
