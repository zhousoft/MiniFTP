#include "ftpproto.h"
#include "sysutil.h"
#include "str.h"
void handle_child(session_t *sess)
{

	writen(sess->ctrl_fd, "220 (miniftpd 0.1 hello)\r\n", strlen("220 (miniftpd 0.1 hello)\r\n"));//客户端连接时服务器先发送220
	int ret;
	while(1)
	{
		memset(sess->cmdline, 0, sizeof(sess->cmdline));
		memset(sess->cmd, 0, sizeof(sess->cmd));
		memset(sess->arg, 0, sizeof(sess->arg));
		ret = readline(sess->ctrl_fd, sess->cmdline, MAX_COMMAND_LINE);//读取客户端命令
		if(ret == -1)//获取客户端命令失败
			ERR_EXIT("readline");
		else if(ret == 0)//客户端主动关闭
			ERR_EXIT(EXIT_SUCCESS);

		printf("cmdline = [%s]\n",sess->cmd);
		//去除接收到的命令行末尾的\r\n
		str_trim_crlf(sess->cmdline);
		//解析FTP命令与参数
		str_split(sess->cmdline, sess->cmd, sess->arg, ' ');
		//将命令转换为大写，便于比较-参数不做处理
		str_upper(sess->cmd);
		//处理FTP命令

		
	}
}
