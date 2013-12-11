#ifndef _NGX_CYCLE_H_INCLUDED_
#define _NGX_CYCLE_H_INCLUDED_

void ngx_create_pidfile(const char *);
void ngx_delete_pidfile();
int ngx_signal_process(const char *);

#endif
