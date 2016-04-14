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
	if(host != NULL)
	{
		if(inet_aton(host, &servaddr.sin_addr) == 0)/*转换失败，host不是有效的IP地址,可能提供的是主机名*/
		{
			struct hostent *hp;
			if((hp = gethostbyname(host)) == NULL)/*通过主机名获取本机上所有IP地址*/
				ERR_EXIT("gethostbyname");

			servaddr.sin_addr = *(struct in_addr*)hp->h_addr;//获取主机IP列表中第一个IP地址

		}
	}
	else/*主机名为空,获取本机任意ip地址*/
	{
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}

	servaddr.sin_port = htons(port);

	int on = 1;
	if((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on,sizeof(on))) <0)/*设置地址复用*/
	{
		ERR_EXIT("gethostbyname-setreuse");
	}
	
	if(bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) <0)
	{
		ERR_EXIT("gethostbyname-bind");
	}

	if(listen(listenfd, SOMAXCONN) <0)
	{
		ERR_EXIT("gethostbyname-listen");
	}

	return listenfd;

}


int getlocalip(char *ip)
{
	char host[100] = {0};
	if(gethostname(host,sizeof(host)) < 0)
		return -1;
	struct hostent *hp;
	if((hp = gethostbyname(host)) == NULL)
		return -1;
	
	strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr));
		return 0;
}

/**
* activate_noblock -设置I/O为非阻塞模式
* @fd 文件描述符
*/
void activate_nonblock(int fd)
{
	int ret;
	int flags = fcntl(fd,F_GETFL);
	if(flags == -1)
		ERR_EXIT("fcntl");
	flags |= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if(ret == -1)
		ERR_EXIT("fcntl");

}


/**
* deactivate_nonblock -设置I/O为阻塞模式
* @fd: 文件描述符
*/
void deactivate_nonblock(int fd)
{
	int ret;
	int flags = fcntl(fd,F_GETFL);
	if(flags == -1)
		ERR_EXIT("fcntl");
	
	flags &= ~O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if(ret == -1)
		ERR_EXIT("fcntl");
}



/**
* read_timeout -读超时检测函数，不含读操作
* @fd: 文件描述符
* @wait_seconds: 等待超时秒数，如果为0表示不检测超时
* 成功（未超时）返回0，失败返回-1，超时返回-1并且errno = ETIMEOUT
*/
int read_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if(wait_seconds > 0)
	{
		fd_set read_fdset;
		struct timeval timeout;

		FD_ZERO(&read_fdset);
		FD_SET(fd,&read_fdset);

		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			ret = select(fd + 1, &read_fdset, NULL, NULL, &timeout);
		}while(ret < 0 && errno == EINTR);

		if(ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		}
		else
		{
			if(ret == 1)
				ret = 0;
		}
	}
	return ret;
}

/*
* write_timeout-写超时检测函数，不含写操作
* @fd: 文件描述符
* @wait_seconds: 等待超时秒数，为0表示不检测超时
* 成功（未超时）返回0，失败返回-1，超时返回-1并且errno=ETIMEDOUT
*/
int write_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if(wait_seconds > 0)
	{
		fd_set write_fdset;
		struct timeval timeout;
		
		FD_ZERO(&write_fdset);
		FD_SET(fd, &write_fdset);

		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		do
		{
			ret = select(fd + 1, NULL, &write_fdset, NULL, &timeout);
		}while(ret < 0 && errno == EINTR);

		if(ret == 0)
		{
			ret = -1;
			errno = ETIMEDOUT;
		}
		else
		{
			if(ret == 1)
				ret = 0;
		}
	}

	return ret;

}


/**
* accept_timeout - 带超时的accept
* @fd: 套接字
* @addr: 输出参数，返回发起连接的对方地址
* @wait_seconds: 等待超时秒数，为0表示正常模式
* 成功（未超时）返回已连接套接字，超时返回-1并且errno = ETIMEDOUT
*/
int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	if(wait_seconds > 0)
	{
		fd_set accept_fdset;
		struct timeval timeout;
		FD_ZERO(&accept_fdset);
		FD_SET(fd, &accept_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;

		do
		{
			ret = select(fd + 1, &accept_fdset, NULL, NULL, &timeout);
		}while(ret < 0 && errno == EINTR);
		if(ret == -1)
			return -1;
		else
		{
			if(ret == 0)
			{
				errno = ETIMEDOUT;/*连接超时*/
				return -1;
			}
		}
	}

	if(addr != NULL)
		ret = accept(fd, (struct sockaddr*)addr, &addrlen);
	else
		ret = accept(fd, NULL, NULL);

	return ret;
}

/**
* readn-读取n个字节
* @fd: 要读取文件描述符
* @buf: 输出参数，保存读取结果
* @count: 要读取字节数
* 读取成功返回实际读取字节数，失败返回-1
*/
ssize_t readn(int fd, void *buf, size_t count)
{
	size_t nleft = count;//剩余要读取字节数
	ssize_t nread;
	char *bufp = (char*)buf;

	while(nleft > 0)
	{
		if((nread = read(fd, bufp, nleft)) < 0)//读取失败
		{
			if(errno == EINTR)//读取被系统信号中断，继续读取
				continue;
			return -1;//读取失败返回
		}
		else
		{
			if(nread == 0)
				return count - nleft;
		}

		bufp += nread;//接收缓冲区指针后移
		nleft -= nread;//剩余要读取字节数
	}

	return count;
}
/**
* writen - 写n个字节
* @fd: 要写入的文件描述符
* @buf: 要写的数据缓冲区
* @count: 要写字节数
* 写入成功返回实际写入字节数，失败返回-1
*/
ssize_t writen(int fd, const void *buf, size_t count)
{
	size_t nleft = count;
	size_t nwritten;
	char *bufp = (char*)buf;

	while(nleft > 0)
	{
		if((nwritten = write(fd, bufp, nleft)) < 0)
		{
			if(errno == EINTR)
				continue;
			return -1;
		}
		else
		{
			if(nwritten == 0)
				continue;
		}

		bufp += nwritten;
		nleft -= nwritten;
	}
	return count;
}

/**
* recv_peek - 窥探读缓存中的数据，不清除
* @sockfd: 要窥探的socket
* @buf: 输出参数 保存读到数据
* @len: 要读取长度
* 窥探socket读缓存中的数据，不清除读缓存，成功返回缓存中字节数 失败返回-1 对方关闭连接返回0
*/

ssize_t recv_peek(int sockfd, void *buf, size_t len)
{
	while(1)
	{
		int ret = recv(sockfd, buf, len, MSG_PEEK);//MSG_PEEK 窥探读缓存中的数据 不清除
		if(ret == -1 && errno == EINTR)
			continue;
		return ret;
	}
}

/**
* readline - 读取socket读缓存中的一行（\n结尾）
* @sockfd: 要读取的socket
* @buf: 输出参数，保存读取到的数据
* @maxline: 最多读取字节数
* 读取一行，失败返回-1，对方关闭连接返回0
*/

ssize_t readline(int sockfd, void *buf, size_t maxline)
{
	int ret;
	int nread;
	char *bufp = buf;
	int nleft = maxline;
	
	while(1)
	{
		ret = recv_peek(sockfd, bufp, nleft);
		if(ret < 0)//读取失败
			return ret;
		else
		{
			if(ret == 0)//对方关闭连接
				return ret;
		}

		nread = ret;
		int i;
		for(i=0; i<nread; i++)
		{
			if(bufp[i] == '\n')
			{
				ret = readn(sockfd, bufp, i+1);//将sockt读缓存中的数据取出;
				if(ret != i+1)
					exit(EXIT_FAILURE);
				return ret;
			}
		}

		if(nread > nleft)
			exit(EXIT_FAILURE);

		nleft -= nread;
		ret = readn(sockfd, bufp, nread);//取出缓存中已窥探到的数据
		if(ret != nread)
			exit(EXIT_FAILURE);
		bufp += nread;
	}
	return -1;
}
