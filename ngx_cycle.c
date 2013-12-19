#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>//for kill()
#include <string.h>//for strcmp()
#include "ngx_config.h"
#include "ngx_log.h"

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
