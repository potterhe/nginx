#ifndef _NGX_CONNECTION_H_INCLUDED_
#define _NGX_CONNECTION_H_INCLUDED_

#include <netinet/in.h>
#include <sys/socket.h>
#include "ngx_core.h"

typedef struct ngx_listening_s ngx_listening_t;

struct ngx_listening_s {
	/** 
	 * 监听socket对应的打开的文件描述符
	 * 结构内存区域分配后会初始化为 0
	 * 写入配置参数后赋值 -1
	 * 端口网络打开后，为打开的文件描述符
	 */
	int fd;
	struct sockaddr *sockaddr;
	socklen_t		socklen;

	/* socket()的第二个参数*/
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

	/* 连接上的读事件 */
	ngx_event_t		*read;
	int fd;//网络连接对应的打开的文件描述符
};

ngx_connection_t * ngx_get_connection(int);
void ngx_free_connection(ngx_connection_t *);

ngx_listening_t * ngx_create_listening(ngx_conf_t *, void *, socklen_t);
int ngx_open_listening_sockets(ngx_cycle_t *);

#endif
