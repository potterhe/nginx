#ifndef _NGX_PROCESS_CYCLE_H_INCLUDED_
#define _NGX_PROCESS_CYCLE_H_INCLUDED_

#define NGX_CMD_QUIT	    3
#define NGX_CMD_TERMINATE   4

#define NGX_PROCESS_MASTER     1
#define NGX_PROCESS_WORKER     3

void ngx_master_process_cycle();

#endif
