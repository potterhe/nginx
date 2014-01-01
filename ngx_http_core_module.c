#include "ngx_core.h"

char *
ngx_http_core_listen(ngx_conf_t *cf)
{
	//这里模拟从配置文件中解析出来的listen命令
	char *value[] = {"listen", "8081"};
}
