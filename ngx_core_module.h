/**
 * 源程序中并没有这个文件
 * 没有函数原型声明，编译的程序会出现段错误
 */
#ifndef NGX_CORE_MODULE_H_INCLUDED_
#define NGX_CORE_MODULE_H_INCLUDED_

void * ngx_core_module_create_conf(ngx_cycle_t *);
void ngx_core_module_init_conf(ngx_cycle_t *);

#endif;
