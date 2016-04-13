#include "sysutil.h"
/**
 * tcp_server -启动tcp服务器
 * @host: 服务器IP地址或者服务器主机名
 * @port: 服务器端口
 * 成功返回监听套接字
*/
int tcp_server(const char *host, unsigned short port)
{
	int listenfd;
	if((listenfd = socket(PF_INET, SOCK_STREAM,0)) < 0)
		ERR_EXIT("tcp_server");
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;

}
