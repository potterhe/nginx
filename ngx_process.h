#ifndef _NGX_PROCESS_H_INCLUDED_
#define _NGX_PROCESS_H_INCLUDED_

#define NGX_MAX_PROCESSES 1024
typedef void (*ngx_spawn_proc_pt) ();

#include <unistd.h>//for pid_t
typedef struct {
    pid_t   pid;
    int	    ipcfd;
    unsigned int exited;//进程已经由waitpid回收
} ngx_process_t;

void ngx_init_processes_array();
int ngx_init_signals();
pid_t ngx_spawn_process(ngx_spawn_proc_pt);

#endif
