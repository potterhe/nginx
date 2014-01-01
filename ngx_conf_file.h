#ifndef _NGX_CONF_FILE_H_INCLUDED_
#define _NGX_CONF_FILE_H_INCLUDED_

struct ngx_conf_s {
	ngx_cycle_t	*cycle;
};

char * ngx_conf_parse(ngx_conf_t *);
void * ngx_get_conf(void *);

#endif
