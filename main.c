#include "common.h"
#include "sysutil.h"
#include "session.h"
#include "parseconf.h"
#include "tunable.h"
int main()
{
	

parseconf_load_file(MINIFTP_CONF);//加载配置文件

#ifdef DEBUG
	//打印配置信息
    printf("pav=%d\n",tunable_pasv_enable);
	printf("port=%d\n",tunable_port_enable);
	printf("listenport=%u\n",tunable_listen_port);
	printf("maxclients=%u\n",tunable_max_clients);
	printf("maxperip=%u\n",tunable_max_per_ip);
	printf("accepttimeout=%u\n",tunable_accept_timeout);
	printf("connectimeout=%u\n",tunable_connect_timeout);
	printf("idletimeout=%u\n",tunable_idle_session_timeout);
	printf("datatiemout=%u\n",tunable_data_connection_timeout);
	printf("localumask=0%o\n",tunable_local_umask);
	printf("uploadrate=%u\n",tunable_upload_max_rate);
	printf("downloadrate=%u\n",tunable_download_max_rate);
#endif

if(tunable_listen_address == NULL)
		printf("listen address = NULL");
	else
		printf("listen address = %s\n",tunable_listen_address);

	if(getuid() != 0)//如果不是root用户启动
	{
		fprintf(stderr,"miniftpd: must be started ad root\n");
		exit(EXIT_FAILURE);
	}
	
	session_t sess = 
	{
		/*控制连接*/
		0,-1,"","","",
		/*数据连接*/
		NULL, -1, -1,
		/*父子进程通道*/
		-1,-1,
		/*FTP协议状态*/
		0
	};

	signal(SIGCHLD, SIG_IGN);//忽略SIGCHLD信号，避免僵尸进程

	int listenfd = tcp_server(tunable_listen_address, tunable_listen_port);/*创建服务器*/
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
