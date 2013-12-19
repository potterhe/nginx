#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "ngx_process_cycle.h"
#include "ngx_log.h"
#include "ngx_cycle.h"
#include "ngx_connection.h"

extern int worker_ipcfd;
extern int ngx_quit;
extern int ngx_terminate;
extern unsigned int ngx_process;

void
ngx_process_events_and_timers()
{
    ssize_t n;
    int command;

	if (ngx_process == NGX_PROCESS_SINGLE) {
		ngx_log_error("single blocked at pause()");
		pause();
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

void
ngx_event_init_conf(ngx_cycle_t *cycle)
{
	/* TODO 解析配置文件，为全局配置赋值
	 * 这里是一个伪实现，将连接池的大小设置为10
	 */
	cycle->connection_n = 10;
}

int
ngx_event_module_init(ngx_cycle_t *cycle)
{

}

int
ngx_event_process_init(ngx_cycle_t *cycle)
{
	unsigned int i;
	ngx_connection_t *c, *next;
	/**
	 * TODO 初始化连接池
	 * 为连接池分配连续的内存
	 */
	cycle->connections = malloc(sizeof(ngx_connection_t) * cycle->connection_n);
	if (cycle->connections == NULL) {
		return -1;
	}

	//使用data域将所有连接串行链表
	c = cycle->connections;
	i = cycle->connection_n;
	next = NULL;
	
	do {
		i--;
		c[i].data = next;
		c[i].fd = -1;

		next = &c[i];
	} while (i);

	//当前所有连接都是空闲连接,所以简单地把空闲链表指向上面的连接链表,空闲连接数即所有连接的数量
	cycle->free_connections = next;
	cycle->free_connection_n = cycle->connection_n;

	return 0;
}
