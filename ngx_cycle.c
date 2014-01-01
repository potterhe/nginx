#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>//for kill()
#include <string.h>//for strcmp()
#include "ngx_config.h"
#include "ngx_log.h"
#include "ngx_cycle.h"
#include "ngx_connection.h"
#include "ngx_core_module.h"
#include "ngx_conf_file.h"

ngx_cycle_t *ngx_cycle;

ngx_cycle_t *
ngx_init_cycle(ngx_cycle_t *old_cycle)
{
	void *rv;
	ngx_conf_t	conf;
	ngx_cycle_t *cycle;
	unsigned int n;

	/**
	 * 源代码中是通过malloc分配了另外的内存以存储新的配置,如重载配置文件(-s reload)这样的场景
	 * 这里简单地将新配置指向旧的结构
	 */
	cycle = old_cycle;

	/**
	 * 为监听端口分配内存,默认分配10
	 * 网络打开的时候是使用的这里存储的值
	 * 但赋值是在配置文件解析流程中进行的
	 */
	n = old_cycle->listening.nelts ? old_cycle->listening.nelts : 10;
	cycle->listening.elts = malloc(sizeof(ngx_listening_t) * n);
	memset(cycle->listening.elts, 0, sizeof(ngx_listening_t) * n);
	cycle->listening.nelts = n;

	/**
	 * TODO 
	 * NGX_CORE_MODULE
	 * ngx_modules[i]->create_conf()
	 */
	rv = ngx_core_module_create_conf(cycle);
	cycle->ccf = rv;

	conf.cycle = cycle;

	//ngx_conf_param();

	//解析配置文件
	ngx_conf_parse(&conf);

	/**
	 * TODO 
	 * NGX_CORE_MODULE
	 * ngx_modules[i]->init_conf()
	 */
	ngx_core_module_init_conf(cycle);

	/**
	 * TODO 打开监听端口
	 */

	/**
	 * TODO
	 * ngx_modules[i]->init_module()
	 * 调用nginx各模块的init_module()方法
	 * 其中事件模块是ngx_event_module_init()
	 */
	ngx_event_module_init(cycle);
	return cycle;
}

void
ngx_create_pidfile(const char *name)
{
    int fd;
    size_t len;
    char pid[NGX_INT64_LEN + 2];//该长度的缓冲区支持存储64位整数的pid_t,加两字节,是为了存储"CR"(windows),"LF".

    fd = open(name, O_RDWR|O_CREAT, 0644);

    /* WARN: pid_t类型在各平台的定义可能不同,这里使用"%d"是不妥的,
     *	    nginx 使用自己实现的 ngx_snprintf()进行了处理.
     */
    len = snprintf(pid, NGX_INT64_LEN + 2, "%d", getpid());

    /**
     * +1 是为了把缓冲区末尾的\0(snprintf的行为)写入文件,
     * 在从文件中解析pid时，是使用atoi()来处理的
     */
    write(fd, pid, len + 1);
    /* $ od -t a nginx.pid */
    close(fd);
}

void
ngx_delete_pidfile()
{
    unlink(NGX_PID_PATH);
}

int 
ngx_signal_process(const char *sig)
{
    int fd, pid;
    ssize_t n;
    char buf[NGX_INT64_LEN + 2];

    /* get pid from pidfile */
    if ((fd = open(NGX_PID_PATH, O_RDONLY)) == -1) {
		ngx_log_stderr("open pidfile failed");
		return -1;
    }
    
    if ((n = read(fd, buf, NGX_INT64_LEN + 2)) == -1) {
		close(fd);
		return -1;
    }
    
    close(fd);
    ngx_log_error("pid file content %s", buf);

    pid = atoi(buf);
    if (pid == -1) {
		ngx_log_stderr("invalid pid");
		return -1;
    }

    if (strcmp(sig, "stop") == 0) {
		kill(pid, SIGTERM);
		return 0;
    }
    return -1;
}
