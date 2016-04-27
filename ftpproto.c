#include "ftpproto.h"
#include "sysutil.h"
#include "str.h"
#include "ftpcodes.h"
#include "tunable.h"
#include "privsock.h"
void ftp_reply(session_t *sess, int status, const char *text);
void ftp_lreply(session_t *sess, int status, const char *text);

int list_common(session_t *sess, int detail);

int get_port_fd(session_t *sess);
int get_pasv_fd(session_t *sess);
int port_active(session_t *sess);
int pasv_active(session_t *sess);
int get_transfer_fd(session_t *sess);

static void do_user(session_t *sess);
static void do_pass(session_t *sess);
static void do_cwd(session_t *sess);
static void do_cdup(session_t *sess);
static void do_quit(session_t *sess);
static void do_port(session_t *sess);
static void do_pasv(session_t *sess);
static void do_type(session_t *sess);
static void do_stru(session_t *sess);
static void do_mode(session_t *sess);
static void do_retr(session_t *sess);
static void do_stor(session_t *sess);
static void do_appe(session_t *sess);
static void do_list(session_t *sess);
static void do_nlst(session_t *sess);
static void do_rest(session_t *sess);
static void do_abor(session_t *sess);
static void do_pwd(session_t *sess);
static void do_mkd(session_t *sess);
static void do_rmd(session_t *sess);
static void do_dele(session_t *sess);
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_site(session_t *sess);
static void do_syst(session_t *sess);
static void do_feat(session_t *sess);
static void do_size(session_t *sess);
static void do_stat(session_t *sess);
static void do_noop(session_t *sess);
static void do_help(session_t *sess);

typedef struct ftpcmd
{
	const char *cmd;
	void (*cmd_handler)(session_t *sess);
}ftpcmd_t;

static ftpcmd_t ctrl_cmds[] = {
	//访问控制命令
	{"USER",	do_user},
	{"PASS",	do_pass},
	{"CWD",		do_cwd },
	{"XCWD",	do_cwd },
	{"CDUP",	do_cdup},
	{"XCUP",	do_cdup},
	{"QUIT",	do_quit},
	{"ACCT",	NULL   },
	{"SMNT",	NULL   },
	//传输参数命令
	{"PORT",	do_port},
	{"PASV",	do_pasv},
	{"TYPE",	do_type},
	{"STRU",	do_stru},
	{"MODE",	do_mode},

	//服务命令
	{"RETR",	do_retr},
	{"STOR",	do_stor},
	{"APPE",	do_appe},
	{"LIST",	do_list},
	{"NLST",	do_nlst},
	{"REST",	do_rest},
	{"ABOR",	do_abor},
	{"\337\364\377\362ABOR", do_abor},
	{"PWD",		do_pwd },
	{"XPWD",	do_pwd },
	{"MKD",		do_mkd },
	{"XMKD",	do_mkd },
	{"RMD",		do_rmd },
	{"XRMD",	do_rmd },
	{"DELE",	do_dele},
	{"RNFR",	do_rnfr},
	{"RNTO",	do_rnto},
	{"SITE",	do_site},
	{"SYST",	do_syst},
	{"FEAT",	do_feat},
	{"SIZE",	do_size},
	{"STAT",	do_stat},
	{"NOOP",	do_noop},
	{"HELP",	do_help},
	{"STOU",	NULL   },
	{"ALLO",    NULL   }

};



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

		//去除接收到的命令行末尾的\r\n
		str_trim_crlf(sess->cmdline);
		printf("cmdline = [%s]\n",sess->cmdline);
		//解析FTP命令与参数
		str_split(sess->cmdline, sess->cmd, sess->arg, ' ');
		//将命令转换为大写，便于比较-参数不做处理
		str_upper(sess->cmd);
		//处理FTP命令
		/*if(strcmp("USER", sess->cmd) == 0)
		{
			do_user(sess);
		}
		else if(strcmp("PASS", sess->cmd) == 0)
		{
			do_pass(sess);
		}*/
		int i;
		int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]);
		for(i = 0; i<size; i++)
		{
			if(strcmp(ctrl_cmds[i].cmd, sess->cmd) == 0)
			{
				if(ctrl_cmds[i].cmd_handler != NULL)
				{
					ctrl_cmds[i].cmd_handler(sess);
				}
				else
				{
					ftp_reply(sess, FTP_COMMANDNOTIMPL, "Unimplement command.");	
				}
				break;
			}
		}

		if (i == size)
		{
			ftp_reply(sess, FTP_BADCMD, "Unkonw command.");
		}

		
	}
}


void ftp_reply(session_t *sess, int status, const char *text)
{
	char buf[1024] = {0};
	sprintf(buf, "%d %s\r\n", status, text);
	writen(sess->ctrl_fd,buf,strlen(buf));
}

void ftp_lreply(session_t *sess, int status, const char *text)
{
	char buf[1024] = {0};
	sprintf(buf, "%d-%s\r\n", status, text);
	writen(sess->ctrl_fd,buf,strlen(buf));

}

int list_common(session_t *sess, int detail)
{
	DIR *dir = opendir(".");
	if(dir == NULL)
	{
		return 0;
	}

	struct dirent *dt;
	struct stat sbuf;
	while((dt = readdir(dir)) != NULL)
	{
		if(lstat(dt->d_name, &sbuf) <0 )
		{
			continue;
		}
		if(dt->d_name[0] == '.')
		{
			continue;
		}

		char buf[1024] = {0};
		if(detail)//传输文件详细信息
		{
			const char *perms = statbuf_get_perms(&sbuf);//获取文件权限信息
		
			int off = 0;
			off += sprintf(buf, "%s", perms);
			off += sprintf(buf + off, "%3d %-8d %-8d ", (int)sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid);
			off += sprintf(buf + off, "%8lu ",(unsigned long) sbuf.st_size);
		
		
			const char * datebuf = statbuf_get_date(&sbuf);//获取文件日期信息
			off += sprintf(buf + off, "%s ", datebuf);
			if(S_ISLNK(sbuf.st_mode))
			{
				char tmp[1024] = {0};
				readlink(dt->d_name, tmp, sizeof(tmp));
				off += sprintf(buf + off, "%s -> %s\r\n", dt->d_name, tmp);
			}
			else
			{
				off += sprintf(buf + off, "%s\r\n", dt->d_name);
			}
		}
		else//只需要传输文件名
		{	
			sprintf(buf, "%s\r\n", dt->d_name);
		}
		writen(sess->data_fd, buf, strlen(buf));
	}
	closedir(dir);
	return 1;
	
}


int port_active(session_t *sess)
{
	if(sess->port_addr)
	{
		if(pasv_active(sess))
		{
			fprintf(stderr,"both port and pasv are active.");
			exit(EXIT_FAILURE);
		}
		return 1;
	}
	else
	{
		return 0;
	}
}


int pasv_active(session_t *sess)
{
	/*if(sess->pasv_listen_fd != -1)
	{
		if(port_active(sess))
		{
			fprintf(stderr,"both port and pasv are active.");
			exit(EXIT_FAILURE);
		}
		return 1;
	}*/
	//监听套接字由nobody进程创建，服务进程需要向nobody进程询问是否激活pasv模式
	priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_PASV_ACTIVE);
	int active = priv_sock_get_int(sess->child_fd);
	if(active)
	{
		if(port_active(sess))
		{
			fprintf(stderr,"both port and pasv are active.");
			exit(EXIT_FAILURE);
		}
		return 1;	
	}
	return 0;
}

int get_port_fd(session_t *sess)
{
	//向nobody进程发送PRIV_SOCK_GET_DATA_SOCK命令   1
	//向nobody发送一个整数port						4
	//向nobody发送一个字符串ip						不定长
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_GET_DATA_SOCK);
	unsigned short port = ntohs(sess->port_addr->sin_port);
	char *ip = inet_ntoa(sess->port_addr->sin_addr);
	priv_sock_send_int(sess->child_fd, (int)port);
	priv_sock_send_buf(sess->child_fd, ip, strlen(ip));
		
	char res = priv_sock_get_result(sess->child_fd);
	if(res == PRIV_SOCK_RESULT_BAD)
	{
		return 0;
	}
	else if(res == PRIV_SOCK_RESULT_OK)
	{
		sess->data_fd  = priv_sock_recv_fd(sess->child_fd);
	}
	return 1;
}

int get_pasv_fd(session_t *sess)
{
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_ACCEPT);
	char res = priv_sock_get_result(sess->child_fd);
	if(res == PRIV_SOCK_RESULT_BAD)
	{
		return 0;
	}
	else if(res == PRIV_SOCK_RESULT_OK)
	{
		sess->data_fd = priv_sock_recv_fd(sess->child_fd);
	}
	return 1;
}

int get_transfer_fd(session_t *sess)
{
	//检测是否收到PORT或PASV命令
	if(!port_active(sess) && !pasv_active(sess))
	{
		ftp_reply(sess, FTP_BADSENDCONN, "Use PORT or PASV first.");
		return 0;
	}
	int ret = 1;
	//如果是主动模式
	if(port_active(sess))
	{
		//tcp_client(20);//主动模式绑定端口20
	/*	int fd = tcp_client(0);
		if(connect_timeout(fd, sess->port_addr, tunable_connect_timeout) < 0)
		{
			close(fd);
			return 0;
		}

		sess->data_fd = fd;*/

		if(get_port_fd(sess) == 0)
		{
			ret = 0;
		}
		
	}

	//如果是被动模式
	if(pasv_active(sess))
	{
		
	/*	int fd = accept_timeout(sess->pasv_listen_fd, NULL, tunable_accept_timeout);
		close(sess->pasv_listen_fd);
		if(fd == -1)
		{
			return 0;
		}
		sess->data_fd = fd;*/

		if(get_pasv_fd(sess) == 0)
		{
			ret = 0;
		}
	}
	if(sess->port_addr)//已经利用端口信息创建了socket，释放内存
	{
		free(sess->port_addr);
		sess->port_addr = NULL;
	}
	return ret;
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

static void do_cwd(session_t *sess)
{
}
static void do_cdup(session_t *sess)
{
}
static void do_quit(session_t *sess)
{
}
static void do_port(session_t *sess)
{
	//PORT 192,168,1,110,196,170 
	unsigned int v[6];
	sscanf(sess->arg,"%u,%u,%u,%u,%u,%u", &v[2], &v[3], &v[4], &v[5], &v[0], &v[1]);//最后两个数字是端口号高8位低8位，保存在数组开头
	sess->port_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr));
	memset(sess->port_addr, 0 , sizeof(struct sockaddr_in));
	sess->port_addr->sin_family = AF_INET;
	unsigned char *p = (unsigned char *)&sess->port_addr->sin_port;
	p[0] = v[0];
	p[1] = v[1];
	
	p = (unsigned char *)&sess->port_addr->sin_addr;
	p[0] = v[2];
	p[1] = v[3];
	p[2] = v[4];
	p[3] = v[5];

	ftp_reply(sess, FTP_PORTOK, "PORT command successful. Consider using PASV.");

}
static void do_pasv(session_t *sess)
{
	char ip[16] = {0};
	getlocalip(ip);
    
	priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PASV_LISTEN);//向nobody进程发送请求获取监听套接字
	unsigned short port = (int)priv_sock_get_int(sess->child_fd);

	/*sess->pasv_listen_fd = tcp_server(ip, 0);//随机端口
	struct sockaddr_in addr;
	socklen_t addlen = sizeof(addr);
	if(getsockname(sess->pasv_listen_fd, (struct sockaddr *)&addr, &addlen) < 0)
	{
		ERR_EXIT("getsockname");
	}
	unsigned short port = ntohs(addr.sin_port);*/
	unsigned int v[4];
	sscanf(ip,"%u.%u.%u.%u", &v[0], &v[1], &v[2], &v[3]);
	char text[1024] = {0};
	sprintf(text,"Enter Passive Mode (%u,%u,%u,%u,%u,%u).", v[0], v[1], v[2], v[3], port>>8, port&0xFF);
	ftp_reply(sess, FTP_PASVOK, text);
	

}
static void do_type(session_t *sess)
{
	if (strcmp(sess->arg, "A") == 0)
	{
		sess->is_ascii = 1;
		ftp_reply(sess, FTP_TYPEOK, "Switch to ASCII mode.");
	}
	else if (strcmp(sess->arg, "I") == 0)
	{
		sess->is_ascii = 0;
		ftp_reply(sess, FTP_TYPEOK, "Switch to Binary mode.");
	}
	else
	{
		ftp_reply(sess, FTP_BADCMD, "Unrecognised TYPE command.");
	}
}
static void do_stru(session_t *sess)
{
}
static void do_mode(session_t *sess)
{
}
static void do_retr(session_t *sess)
{
}
static void do_stor(session_t *sess)
{
}
static void do_appe(session_t *sess)
{
}
static void do_list(session_t *sess)
{
	//创建数据连接
	if(get_transfer_fd(sess) == 0)
	{
		return;
	}
	//响应150
	ftp_reply(sess, FTP_DATACONN, "Here comes the directory listing.");
	//传输列表
	list_common(sess,1);
	//关闭数据套接字
	close(sess->data_fd);
	sess->data_fd = -1;
	//响应226
	ftp_reply(sess, FTP_TRANSFEROK, "Directory send ok.");
}
static void do_nlst(session_t *sess)
{
	//创建数据连接
	if(get_transfer_fd(sess) == 0)
	{
		return;
	}
	//响应150
	ftp_reply(sess, FTP_DATACONN, "Here comes the directory listing.");
	//传输列表
	list_common(sess,0);
	//关闭数据套接字
	close(sess->data_fd);
	sess->data_fd = -1;
	//响应226
	ftp_reply(sess, FTP_TRANSFEROK, "Directory send ok.");

}
static void do_rest(session_t *sess)
{
}
static void do_abor(session_t *sess)
{
}
static void do_pwd(session_t *sess)
{
	char text[1024] = {0};
	char dir[1024+1] = {0};
	getcwd(dir, 1024);
	sprintf(text,"\"%s\"", dir);
	ftp_reply(sess, FTP_PWDOK,text);
}
static void do_mkd(session_t *sess)
{
}
static void do_rmd(session_t *sess)
{
}
static void do_dele(session_t *sess)
{
}
static void do_rnfr(session_t *sess)
{
}
static void do_rnto(session_t *sess)
{
}
static void do_site(session_t *sess)
{
}
static void do_syst(session_t *sess)
{
	ftp_reply(sess, FTP_SYSTOK, "UNIX Type:L8.");
}
static void do_feat(session_t *sess)
{
	ftp_lreply(sess, FTP_FEAT, "Features:");
	writen(sess->ctrl_fd, "EPRT\r\n", strlen("EPRT\r\n"));
	writen(sess->ctrl_fd, "EPSV\r\n", strlen("EPSV\r\n"));
	writen(sess->ctrl_fd, "MDTM\r\n", strlen("MDTM\r\n"));
	writen(sess->ctrl_fd, "PASV\r\n", strlen("PASV\r\n"));
	writen(sess->ctrl_fd, "REST STREAM\r\n", strlen("REST STREAM\r\n"));
	writen(sess->ctrl_fd, "SIZE\r\n", strlen("SIZE\r\n"));
	writen(sess->ctrl_fd, "TVFS\r\n", strlen("TVFS\r\n"));
	writen(sess->ctrl_fd, "UTF8\r\n", strlen("UTF8\r\n"));
	ftp_reply(sess, FTP_FEAT, "End");
							
}
static void do_size(session_t *sess)
{
}
static void do_stat(session_t *sess)
{
}
static void do_noop(session_t *sess)
{
}
static void do_help(session_t *sess)
{
	
}

