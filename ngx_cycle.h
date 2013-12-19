#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_

typedef struct {
	int daemon;//是否启用“守护进程”模式
	int master;//是否启用master-worker模式
} ngx_core_conf_t;

void ngx_create_pidfile(const char *);
void ngx_delete_pidfile();
int ngx_signal_process(const char *);

#endif
