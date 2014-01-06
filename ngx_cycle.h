#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_

#include "ngx_core.h"
#include "ngx_connection.h"

struct ngx_cycle_s {
	/**
	 * ngx_core_conf_t类型
	 * 指向ngx_core_module的配置结构地址
	 *
	 * nginx各模块的配置数据本是存储于conf_ctx元素中
	 * 以conf_ctx[ngx_module[i]->index] 结构组织
	 * 但当前还没有引入模块。
	 *
	 * 当前以平铺的方式将各模块配置项单独存储，以便于理解
	 */
	void *ccf;

	/**
	 * 当I/O复用机制仅工作在”文件描述符“上时(如select,poll)，
	 * 使用这个数组存储"文件描述符"到"连接"的映射关系,
	 * 以便于根据文件描述符快速找到其对应的连接
	 *
	 * cycle->files[fd] = ngx_connection_t *;
	 */
	ngx_connection_t	**files;
	/**
	 * 系统限制(RLIMIT_NOFILE),允许进程打开文件的最大数量
	 * 这决定了一个进程可以使用的文件描述符的上限
	 */
	unsigned int	files_n;

	/**
	 * 指向空闲连接池链表的首元素
	 * 每当nginx需要新链接的时候，会直接取这个地址，
	 * 然后将指针指向链表的下一个元素(free_connections->data)
	 * 将空闲连接总数(free_connection_n) -1
	 */
	ngx_connection_t *free_connections;
	unsigned int free_connection_n;//空闲连接的总数

	/** 
	 * 源代码使用动态数组(ngx_array_t)存储和组织监听端口数据
	 * 这里简化实现
	 * array of the ngx_listening_t
	 * 存储打开的监听端口配置
	 */
	ngx_listening_t	*listening;

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
