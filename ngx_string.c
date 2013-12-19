#include <stdarg.h>
#include <stdio.h>

/**
 * 需要可变参数支持,调用放在va_start()与va_end()之间
 *
 * va_start(args, fmt);
 * p = ngx_vslprintf(p, last, fmt, args);
 * va_end(args);
 */
char *
ngx_vslprintf(char *buf, char *last, const char *fmt, va_list args)
{
    char *p;
    int d, i;

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

    return buf;
}
