#include "ftpproto.h"
#include "sysutil.h"
#include "str.h"
#include "ftpcodes.h"


void ftp_reply(session_t *sess, int status, const char *text);
static void do_user(session_t *sess);
static void do_pass(session_t *sess);

void handle_child(session_t *sess)
{

	ftp_reply(sess,FTP_GREET, "(miniftpd 0.1)");
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
		if(strcmp("USER", sess->cmd) == 0)
		{
			do_user(sess);
		}
		else if(strcmp("PASS", sess->cmd) == 0)
		{
			do_pass(sess);
		}

		
	}
}


void ftp_reply(session_t *sess, int status, const char *text)
{
	char buf[1024] = {0};
	sprintf(buf, "%d %s\r\n", status, text);
	writen(sess->ctrl_fd,buf,strlen(buf));
}

//处理user命令函数
static void do_user(session_t *sess)//static修饰，只能在当前模块使用
{
	//USER 用户名
	struct passwd *pw = getpwnam(sess->arg);
	if(pw == NULL)
	{
		//用户不存在
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}
	sess->uid = pw->pw_uid;
	ftp_reply(sess, FTP_GIVEPWORD, "Please specify the password.");

}
//处理PASS命令
static void do_pass(session_t *sess)
{
	//PASS 123456
	struct passwd *pw = getpwuid(sess->uid);
	if(pw == NULL)
	{
		//用户不存在
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}

	struct spwd *sp = getspnam(pw->pw_name);//获取保存密码的影子文件
	if(sp == NULL)
	{
	    ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}

	//将明文密码进行加密
	char *encrypted_pass = crypt(sess->arg, sp->sp_pwdp);
	//验证密码
	if(strcmp(encrypted_pass, sp->sp_pwdp) != 0)
	{
		ftp_reply(sess, FTP_LOGINERR, "Login incorrect.");
		return;
	}
	//登陆成功后讲当前进程更改为用户进程
	setegid(pw->pw_gid);
	seteuid(pw->pw_uid);
	chdir(pw->pw_dir);
	//密码正确
	ftp_reply(sess, FTP_LOGINOK, "Login successful.");
}
