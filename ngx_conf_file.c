#include "ngx_cycle.h"

static ngx_core_conf_t ccf={1, 0};

ngx_core_conf_t *
ngx_get_conf()
{
	return &ccf;
}
