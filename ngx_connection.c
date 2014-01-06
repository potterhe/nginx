#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include "ngx_config.h"
#include "ngx_cycle.h"
#include "ngx_connection.h"
#include "ngx_log.h"
#include "ngx_conf_file.h"
#include "ngx_core.h"
#include "ngx_event.h"

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

ngx_connection_t *
ngx_get_connection(int s)
{
	ngx_event_t	*rev;
	ngx_connection_t *c;

	c = ngx_cycle->free_connections;
	if (c == NULL) {
		ngx_log_error("connections are not enough");
		return NULL;
	}

	ngx_cycle->free_connections = c->data;
	ngx_cycle->free_connection_n--;

	/**
	 * 建立fd到连接的关联
	 * 即可以通过“文件描述符”找到其所对应的“连接”对象
	 *
	 * 因为I/O复用机制工作在“文件描述符”上,
	 * 当I/O复用机制告诉我们某个文件描述符上发生了网络事件时，
	 * 需要找到文件描述符所对应的连接对象，以获取更多应用层信息
	 *
	 * 因为文件描述符遵循“最小可用”原则打开，且从0开始,
	 * 所以用数组（文件描述符作索引）来组织是合适的
	 *
	 * 源代码将这个关系存储在全局ngx_cycle结构中
	 */
	ngx_cycle->files[s] = c;

	rev = c->read;
	memset(c, 0, sizeof(ngx_connection_t));
	c->read = rev;
	c->fd = s;

	memset(rev, 0, sizeof(ngx_event_t));
	rev->data = c;

	return c;
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
