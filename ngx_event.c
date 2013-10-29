#include <stdio.h>
#include <unistd.h>

extern int worker_ipcfd;

void
ngx_process_events_and_timers()
{
    char b[1024];

    /*worker process will be blocked here, waiting for master's signal from ipc socket*/
    if (read(worker_ipcfd, b, 1024) > 0) {
	printf("worker process %d revieve msg: %s\n", getpid(), b);
    }
}
