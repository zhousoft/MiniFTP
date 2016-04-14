#include "ftpproto.h"
#include "sysutil.h"

void handle_child(session_t *sess)
{

	writen(sess->ctrl_fd, "220 (miniftpd 0.1 hello)\r\n", strlen("220 (miniftpd 0.1 hello)\r\n"));//客户端连接时服务器先发送220

	while(1)
	{
		memset(sess->cmdline, 0, sizeof(sess->cmdline));
		memset(sess->cmd, 0, sizeof(sess->cmd));
		memset(sess->arg, 0, sizeof(sess->arg));
		readline(sess->ctrl_fd, sess->cmdline, MAX_COMMAND_LINE);//读取客户端命令

		//解析FTP命令与参数
		//处理FTP命令

	}
}
