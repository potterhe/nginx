#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include "ngx_config.h"
#include "ngx_cycle.h"
#include "ngx_connection.h"
#include "ngx_log.h"
#include "ngx_conf_file.h"

extern ngx_cycle_t *ngx_cycle;

ngx_listening_t *
ngx_create_listening(ngx_conf_t *cf, void *sockaddr, socklen_t socklen)
{
	ngx_listening_t *ls;
	struct sockaddr *sa;

	/** 
	 * TODO 找出可用的监听端口存储区域
	 * 这里只是使用第一个元素
	 */
	ls = cf->cycle->listening;

	sa = malloc(socklen);
	bzero(sa, socklen);
	memcpy(sa, sockaddr, socklen);

	ls->sockaddr = sa;
	ls->socklen = socklen;
	ls->fd = -1;
	ls->type = SOCK_STREAM;
	ls->backlog = NGX_LISTEN_BACKLOG;

	return ls;
}

int
ngx_open_listening_sockets(ngx_cycle_t *cycle)
{
	int reuseaddr, s;
	unsigned int i;
	ngx_listening_t *ls;

	/* 监听端口打开时，始终启用SO_REUSERADDR选项 */
	reuseaddr = 1;

	ls = cycle->listening;
	for (i = 0; i < NGX_LISTENING_N; i++) {

		if (ls[i].fd != -1) {
			continue;
		}

		s = socket(ls[i].sockaddr->sa_family, ls[i].type, 0);
		if (s == -1) {
			ngx_log_error("socket() failed!");
			return -1;
		}
		/* TODO 将端口设置为非阻塞 */

		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int));

		if (bind(s, ls[i].sockaddr, ls[i].socklen) == -1) {
			ngx_log_error("bind() failed");
			return -1;
		}

		if (listen(s, ls[i].backlog) == -1) {
			ngx_log_error("listen() failed");
			return -1;
		}

		ls[i].fd = s;
	}
	return 0;
}

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
