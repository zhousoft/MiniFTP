#include "privparent.h"
#include "privsock.h"
#include "session.h"
#include "sysutil.h"
#include "tunable.h"

static void privop_pasv_get_data_sock(session_t *sess);
static void privop_pasv_active(session_t *sess);
static void privop_pasv_listen(session_t *sess);
static void privop_pasv_accept(session_t *sess);

int capset(cap_user_header_t hdrp, const cap_user_data_t datap)//设置进程特权
{
	return syscall(__NR_capset, hdrp, datap);//通过系统调来使用capset设置特权，避免直接使用时编译警告
}
//最小化特权
//将当前进程设置为nobody进程，并设置进程特权，使其可以绑定20端口
void minimize_privilege()
{
	struct passwd *pw = getpwnam("nobody");
	if(pw == NULL)
		return;

		/*将当前进程设置为nobody进程*/
	if(setegid(pw->pw_gid) < 0)
		ERR_EXIT("setegid");
	if(seteuid(pw->pw_uid) < 0)
		ERR_EXIT("seteuid");
	
	struct __user_cap_header_struct cap_header;
	struct __user_cap_data_struct cap_data;
	memset(&cap_header, 0, sizeof(cap_header));
	memset(&cap_data, 0, sizeof(cap_data));
	cap_header.version = _LINUX_CAPABILITY_VERSION_1;//64位系统-32位用1
	cap_header.pid = 0;
	
	__u32 cap_mask = 0;
	cap_mask |= (1 << CAP_NET_BIND_SERVICE);//设置特权位，使能绑定特殊端口
	cap_data.effective = cap_data.permitted = cap_mask;
	cap_data.inheritable = 0;

	capset(&cap_header, &cap_data);

}
void handle_parent(session_t *sess)
{
	
	minimize_privilege();
	char cmd;
	while(1)
	{
		cmd = priv_sock_get_cmd(sess->parent_fd);
		//解析内部命令
		//处理内部命令
		switch(cmd)
		{
		case PRIV_SOCK_GET_DATA_SOCK:
			privop_pasv_get_data_sock(sess);
			break;
		case PRIV_SOCK_PASV_ACTIVE:
			privop_pasv_active(sess);
			break;
		case PRIV_SOCK_PASV_LISTEN:
			privop_pasv_listen(sess);
			break;
		case PRIV_SOCK_PASV_ACCEPT:
			privop_pasv_accept(sess);
			break;
		}
	}
}

static void privop_pasv_get_data_sock(session_t *sess)
{
	//nobody进程接收PRIV_SOCK_GET_DATA_SOCK命令
	//进一步接收一个整数，即port
	//接收字串符，即IP
	unsigned short port = (unsigned short)priv_sock_get_int(sess->parent_fd);
	char ip[16] = {0};
	priv_sock_recv_buf(sess->parent_fd, ip, sizeof(ip));
	//创建套接字，绑定20端口
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(ip);
	int fd = tcp_client(20);//20端口 FTP协议数据传输端口
	printf("get fd = %d\n",fd);
	if(fd == -1)
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	if(connect_timeout(fd, &addr, tunable_connect_timeout) < 0)
	{
		printf("connect err\n");
		close(fd);
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}
	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
	priv_sock_send_fd(sess->parent_fd, fd);
	close(fd);
}
static void privop_pasv_active(session_t *sess)
{
	int active = 0;
	if(sess->pasv_listen_fd != -1)
	{
		active = 1;
	}
	priv_sock_send_int(sess->parent_fd, active);
}
static void privop_pasv_listen(session_t *sess)
{
	char ip[16] = {0};
	getlocalip(ip);
	sess->pasv_listen_fd = tcp_server(ip, 20);//20端口
	struct sockaddr_in addr;
	socklen_t addlen = sizeof(addr);
	if(getsockname(sess->pasv_listen_fd, (struct sockaddr *)&addr, &addlen) < 0)
	{
		ERR_EXIT("getsockname");
	}
	unsigned short port = ntohs(addr.sin_port);
	priv_sock_send_int(sess->parent_fd, (int)port);
}
static void privop_pasv_accept(session_t *sess)
{
	int fd = accept_timeout(sess->pasv_listen_fd, NULL, tunable_accept_timeout);
	close(sess->pasv_listen_fd);
	if(fd == -1)
	{
		priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_BAD);
		return;
	}

	priv_sock_send_result(sess->parent_fd, PRIV_SOCK_RESULT_OK);
	priv_sock_send_fd(sess->parent_fd, fd);
	close(fd);
	
}

