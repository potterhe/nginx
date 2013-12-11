#ifndef _NGX_LOG_H_INCLUDED_
#define _NGX_LOG_H_INCLUDED_

#define NGX_MAX_ERROR_STR 2048

void * ngx_log_init(char *);
void ngx_log_error(const char *, ...);
void ngx_log_stderr(const char *, ...);

#endif
