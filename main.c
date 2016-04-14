#include "common.h"
#include "sysutil.h"
#include "session.h"

int main()
{
	if(getuid() != 0)//如果不是root用户启动
	{
		fprintf(stderr,"miniftpd: must be started ad root\n");
		exit(EXIT_FAILURE);
	}
	
	session_t sess = 
	{
		/*控制连接*/
		-1,"","","",
		/*父子进程通道*/
		-1,-1,
	};
	int listenfd = tcp_server(NULL, 5189);/*创建服务器*/
	int conn;
	pid_t pid;

	while(1)
	{
		/*接受客户端连接*/
		conn = accept_timeout(listenfd, NULL, 0);
		if(conn == -1)
			ERR_EXIT("accept_timeout");

		/*获得连接套接字后开辟新进程进行处理*/
		pid = fork();
		if(pid == -1)
			ERR_EXIT("fork");

		if(pid == 0)
		{
			close(listenfd);/*子进程不需要继续监听，关闭子进程的监听套接字*/
			sess.ctrl_fd = conn;
			begin_session(&sess);/*开启会话-包含两个进程（nobody进程，服务进程）*/
		}
		else
		{
			close(conn);/*父进程（主进程）不处理客户端连接，交给子进程处理*/
		}

	}
	
	return 0;
}
