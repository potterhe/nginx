#ifndef _NGX_EVENT_H_INCLUDED_
#define _NGX_EVENT_H_INCLUDED_

void ngx_process_events_and_timers();
char * ngx_events_block(ngx_conf_t *);

#endif
