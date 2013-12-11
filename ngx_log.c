#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>//for malloc()
#include <stdio.h>
#include "ngx_config.h"
#include "ngx_log.h"

static int ngx_log_fd;

void
ngx_log_error(const char *fmt, ...)
{
    char errstr[NGX_MAX_ERROR_STR];
    ssize_t n;
    va_list args;

    /**
     * TODO 解析可变参数列表
     * 这里只是简单地把fmt写入日志
     */
    n = strlen(fmt);
    memcpy(errstr, fmt, n);

    write(ngx_log_fd, errstr, n);
}

void
ngx_log_stderr(const char *fmt, ...)
{
    char *p, *buf, *last, errstr[NGX_MAX_ERROR_STR];
    int d, i;
    /* [K & R] 1978 B7
     * va_list args 将依次指向每个实际参数 
     */
    va_list args;

    last = errstr + NGX_MAX_ERROR_STR;
    buf = errstr;

    /*
     * 在访问任何未命名的参数前，必须用va_start宏初始化一次
     * va_start(va_list ap, lastarg);
     * lastarg 是最后一个命名的形式参数，
     * 此后，每次执行宏va_arg,都将产生产生一个与下一个未命名的参数具有相同类型的值，
     * 它同时修改ap，以使得下一次执行va_arg时返回下一个参数
     * va_arg(va_list ap, 类型);
     * 在所有的参数处理完毕后，且在退出前，必须调用宏va_end一次。
     * va_end(va_list ap);
     */
    va_start(args, fmt);

    while (*fmt && buf < last) {

	if (*fmt == '%') {
	    fmt++;
	    switch (*fmt) {

		case 's':
		    p = va_arg(args, char *);
		    while (*p && buf < last) {
			//*buf++ = *p++;
			*buf = *p;
			buf++;
			p++;
		    }
		    fmt++;
		    continue;

		case 'c':
		    d = va_arg(args, int);
		    *buf++ = (char) (d & 0xff);
		    fmt++;

		    continue;

		case 'd':
		    d = va_arg(args, int);
		    i = snprintf(buf, last - buf, "%d", d);
		    buf += i;
		    fmt++;
		    continue;

		default:
		    //*buf++ = *fmt++;
		    *buf = *fmt;
		    buf++;
		    fmt++;
		    continue;
	    
	    }
	
	} else {
	    //*buf++ = *fmt++;
	    *buf = *fmt;
	    buf++;
	    fmt++;
	}
    
    }
    va_end(args);

    write(STDERR_FILENO, errstr, buf - errstr);
}

void *
ngx_log_init(char *prefix)
{
    char *p, *name;
    size_t plen, nlen;

    name = NGX_ERROR_LOG_PATH;
    nlen = strlen(name);

    p = NULL;

    /* 如果日志名不是绝对路径，需要加上路径前缀，组装成绝对路径 */
    if (name[0] != '/') {
	if (prefix) {
	    plen = strlen(prefix);
	    
	} else {
	    prefix = NGX_PREFIX;
	    plen = strlen(prefix);
	
	}

	if (plen) {
	    name = malloc(plen + nlen + 2);// +2 是为了连接的'/'和末尾的\0
	    if (name == NULL) {
		return NULL;
	    }

	    memcpy(name, prefix, plen);
	    p = name + plen;

	    //判定前缀路径的最后是否有路径分隔符
	    if ((*(p - 1)) != '/') {
		*p++ = '/';
	    }

	    memcpy(p, NGX_ERROR_LOG_PATH, nlen + 1);// +1会把NGX_ERROR_LOG_PATH末尾的\0复制过来
	}
    }

    ngx_log_fd = open(name, O_CREAT|O_WRONLY|O_APPEND, 0644);
    if (ngx_log_fd == -1) {
	ngx_log_stderr("count not open error log file \"%s\"", name);
	ngx_log_fd = STDERR_FILENO;
    }

    if (p) {
	free(name);
    }

    return &ngx_log_fd;
}
