#ifndef _NGX_EVENT_H_INCLUDED_
#define _NGX_EVENT_H_INCLUDED_

void ngx_process_events_and_timers();
void ngx_events_block(ngx_conf_t *);

int ngx_event_process_init(ngx_cycle_t *);
int ngx_event_module_init(ngx_cycle_t *);

#endif
