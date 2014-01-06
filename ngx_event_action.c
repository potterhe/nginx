#include <stdlib.h>
#include <poll.h>
#include "ngx_event.h"
#include "ngx_cycle.h"
#include "ngx_log.h"
#include "ngx_core.h"

extern ngx_cycle_t *ngx_cycle;

static int ngx_poll_init(ngx_cycle_t *);
static int ngx_poll_add_event(ngx_event_t *, int);
static int ngx_poll_process_events(ngx_cycle_t *);

/* poll()调用需要维护的数据 */
static struct pollfd *event_list;
static int nevents;

static ngx_event_actions_t ngx_poll_event_actions = {
	ngx_poll_add_event,
	ngx_poll_process_events,
	ngx_poll_init
};

/* 这里我们以poll机制来描述流程实现 */
int
ngx_event_actions_init(ngx_cycle_t *cycle)
{
	ngx_poll_init(cycle);
}

static int
ngx_poll_init(ngx_cycle_t *cycle)
{
	struct pollfd *list;

	list = malloc(sizeof(struct pollfd) * cycle->connection_n);

	event_list = list;
	nevents = 0;

	/* 对全局变量赋值，此后，对事件的调用都通过ngx_event_actions来进行 */
	ngx_event_actions = ngx_poll_event_actions;
	return 0;
}

static int
ngx_poll_add_event(ngx_event_t *ev, int event)
{
	ngx_connection_t *c;
	struct pollfd	*pollfd;

	c = ev->data;
	/**
	 * TODO
	 * 在向I/O复用机制添加事件时, 要注意不能把已有的事件覆盖掉.
	 * 即，如果”连接“已添加过”读“事件，则在添加”写“事件时, 仍然要保留”读“事件有效
	 * 这在像poll这样的用位编码事件的机制中，需要小心处理
	 * 不能简单的赋值，而是必须找出已分配给事件所属连接的pollfd,并打开事件的标记位
	 *
	 * 在源代码中，我们看到作者通过同时检查事件所属连接（ev->data）的读写事件指针,
	 * 来判定应用场景
	 *
	 * 当前我们只关注读事件
	 */
	pollfd = &event_list[nevents];
	nevents++;

	pollfd->fd = c->fd;
	if (event == NGX_READ_EVENT) {
		pollfd->events = POLLIN;
	}
	pollfd->revents = 0;

	return 0;
}

static int
ngx_poll_process_events(ngx_cycle_t *cycle)
{
	int ready, revents, i, nready;
	ngx_event_t	*ev;
	ngx_connection_t *c;

	/**
	 * 进程将阻塞在poll()调用，等待网络事件，超时事件，或者调用被中断
	 */
	ready = poll(event_list, nevents, -1);

	/**
	 * 发生了错误
	 * TODO 有可能是被信号中断，需要检查错误码
	 */
	if (ready == -1) {
		ngx_log_error("poll() failed");
		return -1;
	}

	/* 定时器到时之前，没有任何描述符就绪 */
	if (ready == 0) {
		return 0;
	}

	/**
	 * 迭代范围内的文件描述符，
	 * 逐个检查是否有网络事件发生
	 */
	for (i = 0; i < nevents; i++) {

		revents = event_list[i].revents;

		/**
		 * 一个无效的文件描述符出现在了待检测范围内
		 * 这种情况不应该出现
		 */
		if (event_list[i].fd == -1) {
			ngx_log_error("pollfd.fd == -1");
			continue;
		}

		/**
		 * fd2conn
		 * 从这里我们可以看到，I/O复用机制在“文件描述符”上工作，
		 * 它只能告诉我们，那个文件描述符有事件发生，
		 * 在nginx中，应用层通过ngx_connection_t来描述网络连接，
		 * 且"连接"与"文件描述符"一一对应
		 * 因此，我们需要能够通过“文件描述符”找到其对应的"连接“
		 */
		c = ngx_cycle->files[event_list[i].fd];

		/* 这里发生了数据不同步的情况 */
		if (c->fd == -1) {
			event_list[i].fd = -1;
			continue;
		}

		if (revents & POLLIN) {
			ev = c->read;
			ev->handler(ev);
		}
	}
	return 0;
}
