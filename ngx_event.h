#ifndef _NGX_EVENT_H_INCLUDED_
#define _NGX_EVENT_H_INCLUDED_

#include "ngx_core.h"

struct ngx_event_s {
	/* 指向事件所属的连接(ngx_connection_t) */
	void	*data;
	ngx_event_handler_pt	handler;
};

/**
 * 事件模型的抽象接口定义
 *
 * 一般而言，事件模型使用”I/O复用(select,poll,epoll等)“机制来实现
 * 这些机制运行时，一般都包含以下的步骤内容
 *
 * 一些初始化工作
 * 如何添加/删除网络事件(监听端口的读事件)到I/O复用,
 * 在何处等待事件的发生，即进程在何处阻塞
 * 如何接收新的网络连接（由监听端口的读事件触发）
 * 如何接收/发送网络数据
 *
 * 基本每一种I/O复用机制都围绕这些需求来实现
 * nginx通过下面的结构抽象了这个模型，
 * 隐藏了各种I/O复用机制实现上的差异,
 * 以提供统一的编程界面，也方便在不同系统的不同实现上移植。
 *
 */
typedef struct {
	int (*add)(ngx_event_t *, int);
	/* 使进程阻塞的系统调用，如select(),poll(),epoll_wait() */
	int (*process_events)(ngx_cycle_t *);

	int (*init)(ngx_cycle_t *);

} ngx_event_actions_t;

extern ngx_event_actions_t	ngx_event_actions;

#define	ngx_process_events	ngx_event_actions.process_events
#define NGX_READ_EVENT	1
#define ngx_add_event	ngx_event_actions.add

void ngx_process_events_and_timers();
void ngx_events_block(ngx_conf_t *);

int ngx_event_process_init(ngx_cycle_t *);
int ngx_event_module_init(ngx_cycle_t *);

#endif
