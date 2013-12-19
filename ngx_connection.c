#include "ngx_cycle.h"
#include "ngx_connection.h"

extern ngx_cycle_t *ngx_cycle;

/**
 * 将一个连接放回空闲链表，
 * 这个链接总是被放到空闲链表的最前面
 */
void
ngx_free_connection(ngx_connection_t *c)
{
	c->data = ngx_cycle->free_connections;
	ngx_cycle->free_connections = c;
	ngx_cycle->free_connection_n++;
}
