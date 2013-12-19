#include <stdio.h>
#include <unistd.h>
#include "ngx_process_cycle.h"
#include "ngx_log.h"

extern int worker_ipcfd;
extern int ngx_quit;
extern int ngx_terminate;

void
ngx_process_events_and_timers()
{
    ssize_t n;
    int command;

    /*worker process will be blocked here, waiting for master's signal from ipc socket*/
    if ((n = read(worker_ipcfd, &command, sizeof(int))) > 0) {
		ngx_log_error("worker process pid:%d revieve command: %d", getpid(), command);

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
