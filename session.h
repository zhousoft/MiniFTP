#ifndef _SESSION_H_
#define _SESSION_H_

#include "common.h"

typedef struct session
{
	uid_t uid;//保存登陆用户uid
	int ctrl_fd;//控制连接
	char cmdline[MAX_COMMAND_LINE];
	char cmd[MAX_COMMAND];
	char arg[MAX_ARG];
	//父子进程通道
	int parent_fd;
	int child_fd;
}session_t;

void begin_session(session_t *sess);
#endif/*_SESSION_H_*/
