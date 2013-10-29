#include <stdarg.h>
#include <unistd.h>

#define NGX_MAX_ERROR_STR 2048

void
ngx_log_stderr(const char *fmt, ...)
{
    char *p, *buf, *last, errstr[NGX_MAX_ERROR_STR];
    int d;
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
			*buf = *p;
			buf++;
			p++;
		    }
		    fmt++;
		    continue;

		case 'c':
		    d = va_arg(args, int);
		    *buf++ = (u_char) (d & 0xff);
		    fmt++;

		    continue;

		default:
		    *buf = *fmt;
		    buf++;
		    fmt++;
		    continue;
	    
	    }
	
	} else {
	    *buf = *fmt;
	    buf++;
	    fmt++;
	}
    
    }
    va_end(args);

    write(STDERR_FILENO, errstr, buf - errstr);
}
