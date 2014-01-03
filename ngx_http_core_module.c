#include <stdlib.h>
#include "ngx_core.h"
#include "ngx_http_core_module.h"

#include <netinet/in.h>
#include <strings.h>

ngx_http_conf_addr_t *
ngx_http_core_listen(ngx_conf_t *cf)
{
	ngx_http_conf_addr_t *addr;
	struct sockaddr_in *sin;
	int port;

	/**
	 * 这里模拟从配置文件中解析出来的listen命令
	 *
	 * nginx 会根据value[1]的值判定所使用的协议
	 * "unix:" unix domain
	 * "[" INET6
	 * 其它INET
	 *
	 * 下面的配置是INET
	 */
	char *value[] = {"listen", "8081"};

	addr = malloc(sizeof(ngx_http_conf_addr_t));
	bzero(addr, sizeof(ngx_http_conf_addr_t));

	addr->socklen = sizeof(struct sockaddr_in);
	sin = (struct sockaddr_in *)&addr->sockaddr;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = htonl(INADDR_ANY);

	port = atoi(value[1]);
	sin->sin_port = htons(port);

	return addr;
}
