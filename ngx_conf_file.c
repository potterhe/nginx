#include <stdlib.h>
#include "ngx_cycle.h"
#include "ngx_event.h"
#include "ngx_http.h"

static int ngx_conf_handler(ngx_conf_t *);

//配置文件解析入口
char *
ngx_conf_parse(ngx_conf_t *cf)
{
	/**
	 * TODO 
	 * open conf file
	 * for ( ; ; ) {
	 * 	ngx_conf_read_token();
	 * 	ngx_conf_handler();	
	 * }
	 */
	ngx_conf_handler(cf);

	return NULL;
}

static int
ngx_conf_handler(ngx_conf_t *cf)
{
	/**
	 * 对配置文件的解析是由各模块定义的ngx_command_t类型负责处理
	 * 特别地，我们这里想为nginx添加监听端口,即解析"listen"命令
	 * 监听端口由具体的业务模块(如http、mail)负责解析，这是合理的。
	 * 因为当我们开启一个监听端口时，与之关联的业务模型应该是明确的。
	 * 这也是nginx提供的一个很好的抽象
	 * 我们可以把nginx扩展成任何其它的网络服务器，如网络游戏等
	 *
	 * 这里通过http模块的处理方式来了解。
	 * http模块的listen命令由下面的函数解析 
	 */

	/*
	ngx_command_t *cmd;
	name= cf->args->elts;
	cmd->set(); 
	*/

	/**
	 * events {
	 *     worker_connections  64;
	 * }
	 */
	ngx_events_block(cf);

	/**
	 * http {
	 *     server {
	 *         listen       8081;
	 *	   }
	 * }
	 * ngx_http_block();
	 * ngx_http_core_server();
	 * ngx_http_core_listen();
	 */
	ngx_http_block(cf);

	return 0;
}

void *
ngx_get_conf(void *c)
{
	return c;
}
