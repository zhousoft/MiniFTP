#include "common.h"
#include "session.h"
#include "ftpproto.h"
#include "privparent.h"
void begin_session(session_t *sess)
{
 		

	int sockfds[2];
	if(socketpair(PF_UNIX, SOCK_STREAM, 0, sockfds) < 0)
		ERR_EXIT("socktpair");
	

	pid_t pid;
	pid = fork();
	if(pid < 0)
		ERR_EXIT("fork");
	if(pid == 0)
	{
		//ftp服务进程，负责与客户端交互，处理通信细节
		close(sockfds[0]);
		sess->child_fd = sockfds[1];
		handle_child(sess);
	}
	else
	{	
		struct passwd *pw = getpwnam("nobody");
		if(pw == NULL)
			return;

		/*将当前进程设置为nobody进程*/
		if(setegid(pw->pw_gid) < 0)
			ERR_EXIT("setegid");
		if(seteuid(pw->pw_uid) < 0)
			ERR_EXIT("seteuid");
		//nobody进程，与服务进程通信，进行权限控制
		close(sockfds[1]);
		sess->parent_fd = sockfds[0];
		handle_parent(sess);
	}
}
