#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_

#include "ngx_core.h"

struct ngx_cycle_s {
	/*空闲连接池链表的首元素*/
	ngx_connection_t *free_connections;
	unsigned int free_connection_n;//空闲连接的总数

	unsigned int connection_n;//连接总数
	ngx_connection_t *connections;
};

typedef struct {
	int daemon;//是否启用“守护进程”模式
	int master;//是否启用master-worker模式
} ngx_core_conf_t;

ngx_cycle_t * ngx_init_cycle(ngx_cycle_t *);
void ngx_create_pidfile(const char *);
void ngx_delete_pidfile();
int ngx_signal_process(const char *);

#endif
