#include <string.h>//for strcmp()
#include <stdlib.h>
#include "ngx_config.h"
#include "ngx_cycle.h"
#include "ngx_log.h"
#include "ngx_process.h"
#include "ngx_process_cycle.h"
#include "ngx_os.h"
#include "ngx_conf_file.h"

extern unsigned int ngx_process;
extern ngx_cycle_t *ngx_cycle;

static int ngx_get_options(int argc, const char *argv[]);

//下面的变量在解析命令行选项流程中赋值
static char *ngx_prefix; //存储命令行 -p 参数
static char *ngx_signal; //存储命令行 -s 参数

int
main(int argc, const char *argv[])
{
	ngx_cycle_t *cycle, init_cycle;
	ngx_core_conf_t *ccf;

    ngx_get_options(argc, argv);

    //初始化日志
    ngx_log_init(ngx_prefix);

	//初始化配置结构
	memset(&init_cycle, 0, sizeof(ngx_cycle_t));
	cycle = ngx_init_cycle(&init_cycle);
	ngx_cycle = cycle;
	
	if (ngx_signal) {
		return ngx_signal_process(ngx_signal);
	}

	//获取配置信息，这里是一个伪实现
	ccf = (ngx_core_conf_t *) ngx_get_conf(cycle->ccf);
	//根据配置文件，确定使用何种进程模式，默认是单进程模式
	if (ccf->master && ngx_process == NGX_PROCESS_SINGLE) {
		ngx_process = NGX_PROCESS_MASTER;
	}

    ngx_init_signals();
    //daemonized
	if (ccf->daemon) {
		ngx_daemon();
	}

    ngx_create_pidfile(NGX_PID_PATH);

	if (ngx_process == NGX_PROCESS_SINGLE) {
		ngx_single_process_cycle(cycle);
	
	} else {
		ngx_master_process_cycle();
	}

    return 0;
} 

static int
ngx_get_options(int argc, const char *argv[])
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

void *
ngx_core_module_create_conf(ngx_cycle_t *cycle)
{
	ngx_core_conf_t *ccf;

	ccf = malloc(sizeof(ngx_core_conf_t));
	if (ccf == NULL) {
		return NULL;
	}

	ccf->daemon = -1;
	ccf->master = -1;

	return ccf;
}

void
ngx_core_module_init_conf(ngx_cycle_t *cycle)
{
	ngx_core_conf_t *ccf;

	ccf = (ngx_core_conf_t *) cycle->ccf;
	ccf->daemon = 1;
	ccf->master = 0;
}
