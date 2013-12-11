#include "ngx_config.h"

//下面的变量在解析命令行选项流程中赋值
static char *ngx_prefix; //存储命令行 -p 参数
static char *ngx_signal; //存储命令行 -s 参数

int
main(int argc, char *argv[])
{
    ngx_get_options(argc, argv);
    if (ngx_signal) {
	return ngx_signal_process(ngx_signal);
    }
    //初始化日志
    ngx_log_init(ngx_prefix);

    ngx_init_signals();
    //daemonized
    ngx_daemon();

    ngx_create_pidfile(NGX_PID_PATH);
    ngx_master_process_cycle();

    return 0;
} 

int ngx_get_options(int argc, const char *argv[])
{
    int i;
    char *p;

    /* nginx -s stop
     * argv[0] = "nginx"
     * argv[1] = "-s"
     * argv[2] = "stop"
     */
    for (i = 1; i < argc; i++) {

	/* p point "[-]s" */
	p = argv[i];
	if (*p != '-') {
	    ngx_log_stderr("invalid option: \"%s\"", argv[i]);
	    return -1;
	}
	/* p point "-[s]" */
	p++;
	while (*p) {
	    switch (*p++) {

		case 's':
		    if (*p) {
			/* support "nginx -sstop" */
			ngx_signal = p;
		    } else if (argv[++i]) {
			/* nginx -s stop*/
			ngx_signal = argv[i];
		    } else {
			ngx_log_stderr("option \"-s\" requires parameter");
			return -1;
		    }

		    /* validate ngx_signal value */
		    if (strcmp(ngx_signal, "stop") == 0) {
			goto next;
		    }
		    ngx_log_stderr("invalid option: \"-s %s\"", ngx_signal);
		    return -1;

		default:
		    ngx_log_stderr("invalid option: \"%c\"", *(p - 1));
		    return -1;
		    
	    }
	
	}
next:
	continue;
    }

    return 0;
}

