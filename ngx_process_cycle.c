#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdlib.h>
#include "ngx_process.h"
#include "ngx_process_cycle.h"
#include "ngx_log.h"
#include "ngx_cycle.h"
#include "ngx_event.h"

/**
 * 标识当前进程的角色，master, worker, ...
 * 在信号处理时，需要区分，因为master,worker等都共享ngx_signal_handler()这个信号处理函数
 */
unsigned int ngx_process;

int ngx_reap;//回收子进程
int ngx_sigalrm;//master监控worker进程退出的闹钟标记
int ngx_reconfigure;//nginx -s reload
int ngx_terminate;//nginx -s stop
int ngx_quit;

int ngx_daemonized;//守护进程化标识

extern ngx_process_t ngx_processes[NGX_MAX_PROCESSES];

static void ngx_start_worker_processes(int);
static void ngx_worker_process_cycle();
static void ngx_worker_process_init();
static int ngx_worker_process_exit();
static void ngx_signal_worker_processes(int signo);
static unsigned int ngx_reap_children();
static void ngx_master_process_exit();

void
ngx_master_process_cycle()
{
    sigset_t set;
    unsigned int live; //living worker process
    int delay;//单位:毫秒.在通过ipc socket给worker退出command后,master已经等待的时间
    struct itimerval itv;//跟踪worker进程退出的定时器

    /*
     * block a lot signals
     */
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGHUP); 
    sigaddset(&set, SIGTERM);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
		ngx_log_error("sigprocmask() failed");
    }
    sigemptyset(&set);

    /* 在nginx的源代码中没能找到初始化全局数组 ngx_processes 的代码
     * 这里是一个合适的位置
     */
    ngx_init_processes_array();

    ngx_start_worker_processes(2);

    delay = 0;
    live = 1;//有worker 进程处于存活状态

    /*
     * watch worker process
     */
    for ( ; ; ) {

		if (delay) {
			/* 判定是否有定时器触发 */
			if (ngx_sigalrm) {
				delay *= 2;//每次定时器信号后，翻倍定时器时延
				ngx_sigalrm = 0;
			}

			/* 安装定时器,不使用'重复'机制
			 *
			 */
			itv.it_interval.tv_sec = 0;
			itv.it_interval.tv_usec = 0;

			itv.it_value.tv_sec = delay / 1000;//秒
			itv.it_value.tv_usec = (delay % 1000) * 1000;//单位:微秒

			if(setitimer(ITIMER_REAL, &itv, NULL) == -1) {
				ngx_log_error("setitimer() failed");
			}
		}
		
		/* 
		 * master process will be blocked here,
		 * waiting for a signal to wake up
		 */
		sigsuspend(&set);

		/**
		 * 回收子进程,这里的回收是针对全局数组 ngx_processes, 
		 * 操作系统层面的进程回收已经在信号处理函数 ngx_signal_handler() 中完成, 
		 * 并对ngx_processes 的相应条目中进行了标记
		 */
		if (ngx_reap) {
			ngx_reap = 0;
			live = ngx_reap_children();
		}

		/* 没有worker进程存活时，master进程才退出*/
		if (!live && ngx_terminate) {
			ngx_master_process_exit();
		}

		/* 
		 * here, ngx_signal_handler() had returned
		 * ngx_signal_handler() 会根据接收到的信号，设置几个全局变量的值
		 * 我们需要在这里检测这几全局值，正确响应信号约定的行为
		 *
		 * ngx_terminate "nginx -s stop" or "kill -s SIGTERM MASTER_PID"
		 *
		 * for this option, nginx had optimized. 
		 * first, master send command to worker process by IPC socket.
		 * if worker process did not quit in 1000 ms, master send SIGKILL signal.
		 */
		if (ngx_terminate) {
			if(delay == 0) {
				delay = 50;
			}

			if (delay > 1000) {
				ngx_signal_worker_processes(SIGKILL);
			} else { 
				ngx_signal_worker_processes(SIGTERM);
			}
			continue;
		}

		/* 
		 * ngx_reconfigure "nginx -s reload" or "kill -s SIGHUP MASTER_PID"
		 *
		 * 在实现重载配置文件功能时,nginx首先孵化新的worker进程,新进程读取新重的配置文件
		 * 然后向旧的worker进程发送SIGQUIT command,通过IPC socket
		 */
		if (ngx_reconfigure) {
			ngx_reconfigure = 0;

			/*TODO ngx_start_worker_processes()*/
			ngx_signal_worker_processes(SIGQUIT);
		}

    }
}

void
ngx_single_process_cycle()
{
	/**
	 * TODO
	 * ngx_set_environment()
	 * ngx_modules[i]->init_process()
	 */

	for ( ; ; ) {
		ngx_log_error("single cycle");
		ngx_process_events_and_timers();

		if (ngx_terminate) {
			ngx_master_process_exit();
		}
	}
}

static void
ngx_start_worker_processes(int n)
{
    int i;

    for (i = 0; i < n; i++) {
		ngx_spawn_process(ngx_worker_process_cycle);
    }
}

static void
ngx_worker_process_cycle()
{
    //sign self is a worker process
    ngx_process = NGX_PROCESS_WORKER;

    ngx_worker_process_init();

    for ( ; ; ) {
		ngx_process_events_and_timers();

		if (ngx_terminate) {
			ngx_worker_process_exit();
		}
    }
}

static void
ngx_worker_process_init()
{
    sigset_t set;
    int n;

    /*
    ngx_set_environment();
    setpriority();
    setrlimit(RLIMIT_NOFILE);
    setrlimit(RLIMIT_CORE);
    setrlimit(RLIMIT_SIGPENDING);
    setgid
    initgroups
    setuid
    sched_setaffinity
    chdir
    */

    sigemptyset(&set);
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1) {
    
    }

    /* module init*/
    //ngx_modules[i]->init_process();

    /* 关闭全局数组 ngx_processes 中 master 持有的其它进程的ipc socket [0]
     * 在 ngx_spawn_process() 中我们提到,nginx没有关闭 fd[1]
     * 这里的实现,worker进程是关闭master持有的其它进程的 fd[1]，但没有关闭fd[0]
     * 按理说，worker进程应该关闭掉其它进程的fd[0]才对，fd[0]是master发送cmd的端口.
     * 在worker进程中，全局数组ngx_processes应该仅自身的fd[1]打开，其它全关闭.
     */
    for (n = 0; n < NGX_MAX_PROCESSES; n++) {
		if (ngx_processes[n].pid == -1) {
			continue;
		}

		if (ngx_processes[n].ipcfd == -1) {
			continue;
		}

		close(ngx_processes[n].ipcfd);
		ngx_processes[n].ipcfd = -1;
    }

    //ngx_add_channel_event
}

static int
ngx_worker_process_exit()
{
    ngx_log_error("worker: exit");
    exit(0);
}

static void
ngx_signal_worker_processes(int signo)
{
    int i, command;
    /**
     * NGX_SHUTDOWN_SIGNAL QUIT
     * NGX_TERMINATE_SIGNAL TERM, 
     * NGX_REOPEN_SIGNAL    USR1
     * above signo, IPC socket
     */
    switch (signo) {
		case SIGQUIT:
			command = NGX_CMD_QUIT;
			break;

		case SIGTERM:
			command = NGX_CMD_TERMINATE;
			break;

		case SIGUSR1:
			command = 1;
			break;

		default:
			command = 0;
    }

    for (i = 0; i < NGX_MAX_PROCESSES; i++) {
		if (ngx_processes[i].pid == -1)
			continue;

		/* ipc socket*/
		if (command) {
			write(ngx_processes[i].ipcfd, &command, sizeof(int));
			continue;
		}

		kill(ngx_processes[i].pid, signo);
    }
}

/* return the number of "live" worker process*/
static unsigned int
ngx_reap_children()
{
    unsigned int live;
    int i;

    live = 0;
    /* TODO 清理全局数组 ngx_processes 
     */
    for (i = 0; i < NGX_MAX_PROCESSES; i++) {
		if (ngx_processes[i].pid == -1) {
			continue;
		}

		/* 关闭 master 持有的已结束进程的 ipc socket */
		if (ngx_processes[i].exited == 1) {
			close(ngx_processes[i].ipcfd);
			ngx_processes[i].ipcfd = -1;
			ngx_processes[i].pid = -1;
		
		} else {
			live++;
		}
    }
    return live;
}

static void
ngx_master_process_exit()
{
    ngx_delete_pidfile();

    ngx_log_error("master: exit");
    /* 调用所有模块的 exit_master()
     * ngx_modules[i]->exit_master();
     * ngx_close_listening_sockets();
     */
    exit(0);
}
