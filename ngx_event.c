#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "ngx_process_cycle.h"
#include "ngx_log.h"
#include "ngx_cycle.h"
#include "ngx_connection.h"
#include "ngx_conf_file.h"
#include "ngx_config.h"
#include "ngx_event.h"

#include <netinet/in.h>

extern int worker_ipcfd;
extern int ngx_quit;
extern int ngx_terminate;
extern unsigned int ngx_process;

extern ngx_cycle_t	*ngx_cycle;

/* 当前使用的事件模型的功能集 */
ngx_event_actions_t ngx_event_actions;

static void ngx_event_accept(ngx_event_t *);
static void ngx_event_init_conf(ngx_cycle_t *);

void
ngx_process_events_and_timers()
{
    ssize_t n;
    int command;

	if (ngx_process == NGX_PROCESS_SINGLE) {
		ngx_log_error("single blocked at pause()");
		ngx_process_events(ngx_cycle);
	} else
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

/**
 * 解析类似下面的配置段
 * events {
 *		worker_connections 1024
 * }
 */
void
ngx_events_block(ngx_conf_t *cf)
{
	/**
	 * NGX_EVENT_MODULE
	 * ngx_event_module_t->create_conf();
	 */

	/**
	 * NGX_EVENT_MODULE
	 * ngx_event_module_t->init_conf();
	 */
	ngx_event_init_conf(cf->cycle);
}

static void
ngx_event_init_conf(ngx_cycle_t *cycle)
{
	/* TODO 解析配置文件，为全局配置赋值
	 * 这里是一个伪实现，将连接池的大小设置为64
	 */
	cycle->connection_n = 64;
}

int
ngx_event_module_init(ngx_cycle_t *cycle)
{
	return 0;
}

int
ngx_event_process_init(ngx_cycle_t *cycle)
{
	unsigned int i;
	ngx_event_t		*rev;
	ngx_listening_t	*ls;
	ngx_connection_t *c, *next;

	/**
	 * TODO
	 * event{
	 * 		use poll;
	 * }
	 * 根据"use"配置项的值，选择事件模型(I/O复用)，并初始化
	 * 在初始化流程中对全局变量ngx_event_actions赋值
	 * 后续涉及对事件功能的调用，都是通过对ngx_event_actions的引用完成的
	 *
	 * NGX_EVENT_MODULE
	 * ngx_modules[m]->ctx->actions.init()
	 *
	 * 由于目前我们还没有引入模块的概念
	 * 用下面的函数来完成这个流程
	 */
	ngx_event_actions_init(cycle);

	/** 
	 * TODO 通过getrlimit(RLIMIT_NOFILE)获取系统限制
	 * 这里只是简单地使用连接池的数量
	 */
	cycle->files_n = cycle->connection_n;
	cycle->files = malloc(sizeof(ngx_connection_t *) * cycle->files_n);

	/**
	 * 初始化连接池
	 * 为连接池分配连续的内存
	 */
	cycle->connections = malloc(sizeof(ngx_connection_t) * cycle->connection_n);
	if (cycle->connections == NULL) {
		return -1;
	}

	c = cycle->connections;

	/* 为所有连接的读事件分配内存 */
	rev = malloc(sizeof(ngx_event_t) * cycle->connection_n);

	/**
	 * 使用data域将所有连接串成链表
	 * 将各连接的读事件指向内存区域
	 */
	i = cycle->connection_n;
	next = NULL;

	do {
		i--;
		c[i].data = next;
		c[i].read = &rev[i];
		c[i].fd = -1;

		next = &c[i];
	} while (i);

	/**
	 * 当前所有连接都是空闲连接,所以简单地把空闲链表指向上面的连接链表,
	 * 空闲连接数即所有连接的数量
	 */
	cycle->free_connections = next;
	cycle->free_connection_n = cycle->connection_n;

	/**
	 * for each listening socket
	 * 将打开的监听端口添加到事件机制中
	 * 初始化其”读“事件，以接收来自网络的连接
	 */
	ls = cycle->listening;
	for (i = 0; i < NGX_LISTENING_N; i++) {

		if (ls[i].fd <= 0) {
			continue;
		}

		c = ngx_get_connection(ls[i].fd);
		if (c == NULL) {
			return -1;
		}
		rev = c->read;
		rev->handler = ngx_event_accept;

		ngx_add_event(rev, NGX_READ_EVENT);
	}

	return 0;
}

static void
ngx_event_accept(ngx_event_t *ev)
{
	socklen_t	socklen;
	struct sockaddr_in	sa;
	int s;
	ngx_connection_t *c, *lc;

	/**
	 * 从这里的赋值操作可以看到：
	 * 事件，连接，监听端口结构间的互引用关系
	 */
	lc = ev->data;

	socklen = sizeof(sa);
	s = accept(lc->fd, (struct sockaddr *)&sa, &socklen);

	if (s == -1) {
		ngx_log_error("accept() failed!");
		return;
	}

	/**
	 * TODO 将新连接初始化，将其读事件加入事件机制。
	 * 这里只是简单的往客户端发送一条消息，然后断开连接
	 */
	char str[] = "abc";
	write(s, str, strlen(str));
	close(s);
}
