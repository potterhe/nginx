#include <stdio.h>
#include <fcntl.h>

/* 64位整数的10进制表示的字符串的长度，主要用于为这样的值分配合适的缓冲区 */
#define NGX_INT64_LEN   sizeof("-9223372036854775808") - 1

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
    write(fd, pid, len);
    /* $ od -t a nginx.pid */
    close(fd);
}

void
ngx_delete_pidfile(const char *name)
{
    unlink(name);
}
