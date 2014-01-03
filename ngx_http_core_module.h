#ifndef _NGX_HTTP_CORE_MODULE_INCLUDED_
#define _NGX_HTTP_CORE_MODULE_INCLUDED_

#include <netinet/in.h>

typedef struct {
	struct sockaddr sockaddr;
	socklen_t socklen;

} ngx_http_conf_addr_t;

ngx_http_conf_addr_t * ngx_http_core_listen(ngx_conf_t *);

#endif
