#include "common.h"
#include "session.h"
#include "ftpproto.h"
#include "privparent.h"
#include "privsock.h"
void begin_session(session_t *sess)
{
 		

  /*	int sockfds[2];
	if(socketpair(PF_UNIX, SOCK_STREAM, 0, sockfds) < 0)
		ERR_EXIT("socktpair");*/
	
	priv_sock_init(sess);
	pid_t pid;
	pid = fork();
	if(pid < 0)
		ERR_EXIT("fork");
	if(pid == 0)
	{
		//ftp服务进程，负责与客户端交互，处理通信细节
	//	close(sockfds[0]);
	//	sess->child_fd = sockfds[1];
		priv_sock_set_child_context(sess);
		handle_child(sess);
	}
	else
	{	
	//nobody进程，与服务进程通信，进行权限控制
	//	close(sockfds[1]);
	//	sess->parent_fd = sockfds[0];
		priv_sock_set_parent_context(sess);
		handle_parent(sess);
	}
}
