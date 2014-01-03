#include "ngx_core.h"
#include "ngx_connection.h"
#include "ngx_http_core_module.h"

static ngx_listening_t * ngx_http_add_listening(ngx_conf_t *, ngx_http_conf_addr_t *);

void
ngx_http_block(ngx_conf_t *cf)
{
	ngx_http_conf_addr_t *addr;
	/**
	 * NGX_HTTP_MODULE
	 * ngx_modules[i].create_main_conf();
	 * ngx_modules[i].create_srv_conf();
	 * ngx_modules[i].create_loc_conf();
	 * 
	 * ngx_modules[i].preconfiguration();
	 */

	/* parse inside the http{} block */
	addr = ngx_http_core_listen(cf);

	ngx_http_add_listening(cf, addr);
}

static ngx_listening_t *
ngx_http_add_listening(ngx_conf_t *cf, ngx_http_conf_addr_t *addr)
{
	ngx_listening_t *ls;

	ls = ngx_create_listening(cf, &addr->sockaddr, addr->socklen);

	return ls;
}
