#include <unistd.h>
#include <fcntl.h>

int
ngx_daemon()
{
    int fd;
    /*
     * [APUE CH13]
     * 至此，子进程继承了父进程的进程组ID，但具有一个新的进程ID,(pid != pgid)
     * 这保证了子进程不是一个进程组的[组长进程](进程ID和进程组ID相同的进程)，
     * 这是调用setsid的必要前提条件
     */
    switch (fork()) {
	case -1:
	    return -1;

	case 0:
	    break;

	default:
	    exit(0);//父进程退出
    }

    /*
     * setsid创建一个新会话.使调用进程：
     * a) 成为新会话的首进程
     * b) 成为一个新进程组的组长进程
     * c) 没有控制终端
     */
    setsid();

    umask(0);

    /*
     * Attach file descriptors 0, 1, 2 to /dev/null.
     */
    fd = open("/dev/null", O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);

    close(fd);

    return 0;
}
