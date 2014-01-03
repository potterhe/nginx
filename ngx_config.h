#ifndef _NGX_CONFIG_H_INCLUDED_
#define _NGX_CONFIG_H_INCLUDED_

#include "ngx_auto_config.h"

/**
 * 64位整数的10进制表示的字符串的长度
 * 主要用于为这样的值分配合适的缓冲区
 */
#define NGX_INT64_LEN   sizeof("-9223372036854775808") - 1

#define NGX_LISTENING_N 10
#define NGX_LISTEN_BACKLOG 64

#endif
