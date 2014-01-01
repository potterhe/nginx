#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_

#include <netinet/in.h>
#include <sys/socket.h>
#include "ngx_core.h"

typedef struct ngx_listening_s ngx_listening_t;

struct ngx_listening_s {
	int fd;//监听socket对应的打开的文件描述符

	struct sockaddr *sockaddr;
	socklen_t		socklen;

	int type;

	int backlog;
};

struct ngx_connection_s {
	/**
	 * 对于空闲连接来说，该指针指向空闲链接的下一个空闲连接，相当于next指针
	 * 当连接被使用时，所指向的数据的意义由使用它的nginx模块决定,
	 * 在http中，data指向ngx_http_request_t
	 */
	void *data;
	int fd;//网络连接对应的打开的文件描述符
};

void ngx_free_connection(ngx_connection_t *);

#endif
