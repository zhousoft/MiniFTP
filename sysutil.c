#include "sysutil.h"
int tcp_client(unsigned short port)
{
	int sock;
	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		ERR_EXIT("tcp_client");

	if(port > 0)
	{
		int on = 1;
		if((setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on))) < 0)
			ERR_EXIT("setsockopt");
		char ip[16] = {0};
		getlocalip(ip);
		struct sockaddr_in localaddr;
		memset(&localaddr, 0, sizeof(localaddr));
		localaddr.sin_family = AF_INET;
		localaddr.sin_port = htons(port);
		localaddr.sin_addr.s_addr = inet_addr(ip);
		if(bind(sock, (struct sockaddr*)&localaddr, sizeof(localaddr)) < 0)
			ERR_EXIT("bind");
	}
	return sock;
}
/**
 * tcp_server -启动tcp服务器
 * @host: 服务器IP地址或者服务器主机名
 * @port: 服务器端口
 * 成功返回监听套接字
*/
int tcp_server(const char *host, unsigned short port)
{
#ifdef DEBUG
	printf("start tcp_server, host is %s, port is %d\n",host,port);
#endif
	int listenfd;
	if((listenfd = socket(PF_INET, SOCK_STREAM,0)) < 0)
		ERR_EXIT("tcp_server");
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	if(host != NULL && strcmp(host, "anylocal") != 0)
	{
#ifdef DEBUG
	printf("start server,listen host is %s\n",host);
#endif
		if(inet_aton(host, &servaddr.sin_addr) == 0)/*转换失败，host不是有效的IP地址,可能提供的是主机名*/
		{
			struct hostent *hp;
			if((hp = gethostbyname(host)) == NULL)/*通过主机名获取本机上所有IP地址*/
				ERR_EXIT("gethostbyname");

			servaddr.sin_addr = *(struct in_addr*)hp->h_addr;//获取主机IP列表中第一个IP地址

		}
	}
	else/*主机名为空或指定为anylocal时,监听本机任意ip地址*/
	{
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);//INADDR_ANY(0.0.0.0)表示监听所有网卡信息
#ifdef DEBUG
		printf("server listen ip is %s\n", inet_ntoa(servaddr.sin_addr));
#endif
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

#ifdef DEBUG	
	printf("start get localip\n");
#endif
	
	int sockfd;
	struct ifconf ifconf;
	struct ifreq *ifreq;
    char buf[1024] = {0};
	struct sockaddr_in sin;


	int i;
	char host[100] = {0};
	if(gethostname(host,sizeof(host)) < 0)
	{
		printf("getlocalip err: gethostname fail.\n");
		return -1;
	}
	struct hostent *hp;
	if((hp = gethostbyname(host)) == NULL)
	{
		printf("getlocalip err: gethostbyname fail.\n");
		return -1;
	}
   //gethostbyname()函数解析/etc/hosts文件来获取IP，但是有时该文件内会没有真实IP，
   //因此需要判断下获取的IP是否为真实IP（非127开头的回环IP）

	strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr));
	if(ip[0] == '1' && ip[1] == '2' && ip[2] == '7')
	{
		//hosts文件没有真实IP，读网卡信息来获取
				//初始化ifconf
		ifconf.ifc_len = 1024;
		ifconf.ifc_buf = buf;
		if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			return -1;
		}
		//获取所有接口信息
		ioctl(sockfd, SIOCGIFCONF, &ifconf);
		//逐个获取IP得到可用IP-排除lo地址
		ifreq = (struct ifreq *)buf;
		for(i = (ifconf.ifc_len/sizeof(struct ifreq)); i > 0;i--)
		{
			if(strcmp(ifreq->ifr_name, "lo") == 0)//排除lo本地回环地址
			{
				ifreq++;
				continue;
			}
			memcpy(&sin,&ifreq->ifr_addr, sizeof(sin));
			strcpy(ip, inet_ntoa((struct in_addr)sin.sin_addr));
#ifdef DEBUG
			printf("-ioctlip:%s\n",inet_ntoa((struct in_addr)sin.sin_addr));
#endif
			break;
		}
		close(sockfd);

	}
#ifdef DEBUG
	
	printf("hostname:%s\naddress list:",hp->h_name);
	
	for( i = 0; hp->h_addr_list[i]; i++)
	{
		printf("%s\t",inet_ntoa(*(struct in_addr*)(hp->h_addr_list[i])));
	}
	printf("\ngetlocalip: %s\n",ip);
#endif


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
*
*/

int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
	int ret;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	if(wait_seconds > 0)
		activate_nonblock(fd);
	
	ret = connect(fd, (struct sockaddr*)addr, addrlen);
#ifdef DEBUG
	printf("connect nonblock ret = %d and errno = %d\n",ret,errno);
#endif
	if(ret < 0 && errno == EINPROGRESS)
	{
		fd_set connect_fdset;
		struct timeval timeout;
		FD_ZERO(&connect_fdset);
		FD_SET(fd,&connect_fdset);
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		do
		{
			//一旦连接建立，套接字就可写
			ret = select(fd + 1, NULL, &connect_fdset, NULL, &timeout);
		}while(ret < 0 && errno == EINTR);
		if(ret == 0)
		{
#ifdef DEBUG
			printf("connect time out.\n");
#endif
			errno = ETIMEDOUT;
			ret = -1;
		}
		else if(ret < 0)
		{
#ifdef DEBUG
			printf("connect timeout err\n");
#endif
			return -1;
		}
		else if(ret == 1)
		{
			//ret返回1可能有两种情况，一种是连接建立成功，一种是套接字产生错误
			//此时错误信息不会保存至errno变量中，要调用getsockopt来获取
			int err;
			socklen_t socklen = sizeof(err);
			int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
			if(sockoptret == -1)
			{
#ifdef	DEBUG
				printf("getsocktopt err.\n");
#endif
				return -1;
			}
			if(err == 0)
			{
#ifdef DEBUG
				printf("err == 0\n");
#endif
				ret = 0;
			}
			else
			{	
#ifdef DEBUG 
				printf("connect sockt err.\n");
#endif
				errno = err;
				ret = -1;
			}
		}
	}

	if(wait_seconds > 0)
	{
		deactivate_nonblock(fd);
	}
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

void send_fd(int sock_fd, int fd)
{
	int ret;
	struct msghdr msg;
	struct cmsghdr *p_cmsg;
	struct iovec vec;
	char cmsgbuf[CMSG_SPACE(sizeof(fd))];
	int *p_fds;
	char sendchar = 0;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	p_cmsg = CMSG_FIRSTHDR(&msg);
	p_cmsg->cmsg_level = SOL_SOCKET;
	p_cmsg->cmsg_type = SCM_RIGHTS;
	p_cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
	p_fds = (int *)CMSG_DATA(p_cmsg);
	*p_fds = fd;

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_flags = 0;

	vec.iov_base = &sendchar;
	vec.iov_len = sizeof(sendchar);
	ret = sendmsg(sock_fd, &msg, 0);
	if(ret != 1)
		ERR_EXIT("sendmsg");

}

int recv_fd(const int sock_fd)
{
	int ret;
	struct msghdr msg;
	char recvchar;
	struct iovec vec;
	int recv_fd;
	char cmsgbuf[CMSG_SPACE(sizeof(recv_fd))];
	struct cmsghdr *p_cmsg;
	int *p_fd;
	vec.iov_base = &recvchar;
	vec.iov_len = sizeof(recvchar);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &vec;
	msg.msg_iovlen = 1;
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
	msg.msg_flags = 0;

	p_fd = (int *)CMSG_DATA(CMSG_FIRSTHDR(&msg));
	*p_fd = -1;
	ret = recvmsg(sock_fd, &msg, 0);
	if(ret != 1)
		ERR_EXIT("recvmsg");
	
	p_cmsg = CMSG_FIRSTHDR(&msg);
	if(p_cmsg == NULL)
		ERR_EXIT("no passed fd");

	p_fd = (int*)CMSG_DATA(p_cmsg);
	recv_fd = *p_fd;
	if(recv_fd == -1)
		ERR_EXIT("no passed fd");
	
	return recv_fd;
}
const char *statbuf_get_perms(struct stat *sbuf)
{
	static char perms[] = "----------";
	perms[0] = '?';
	mode_t mode = sbuf->st_mode;
	//获取文件类型
	switch(mode & S_IFMT)
	{
		case S_IFREG:
			perms[0] = '-';
			break;
		case S_IFDIR:
			perms[0] = 'd';
			break;
		case S_IFLNK:
			perms[0] = 'l';
			break;
		case S_IFIFO:
			perms[0] = 'p';
			break;
		case S_IFSOCK:
			perms[0] = 's';
			break;
		case S_IFCHR:
			perms[0] = 'c';
			break;
		case S_IFBLK:
			perms[0] = 'b';
			break;
	}
	//获取权限位
	if(mode & S_IRUSR)
	{
		perms[1] = 'r';
	}
	if(mode & S_IWUSR)
	{
		perms[2] = 'w';
	}
	if(mode & S_IXUSR)
	{
		perms[3] = 'x';
	}
	if(mode & S_IRGRP)
	{
		perms[4] = 'r';
	}
	if(mode & S_IWGRP)
	{
		perms[5] = 'w';
	}
	if(mode & S_IXGRP)
	{
		perms[6] = 'x';
	}
	if(mode & S_IROTH)
	{
		perms[7] = 'r';
	}
	if(mode & S_IWOTH)
	{
		perms[8] = 'w';
	}
	if(mode & S_IXOTH)
	{
		perms[9] = 'x';
	}
	//特殊权限位
	if(mode & S_ISUID)	
	{
		perms[3] = (perms[3] == 'x')?'s' : 'S';
	}
	if(mode & S_ISGID)
	{
		perms[6] = (perms[6] == 'x')?'s' : 'S';
	}
	if(mode & S_ISVTX)
	{
		perms[9] = (perms[9] == 'x')?'t' : 'T';
	}
	
	return perms;
}
const char *statbuf_get_date(struct stat *sbuf)
{
	static char datebuf[64] = {0};
	const char *p_date_format = "%b %e %H:%M";//日期格式
	struct timeval tv;
	gettimeofday(&tv, NULL);
	time_t local_time = tv.tv_sec;
	if(sbuf->st_mtime > local_time ||  (local_time - sbuf->st_mtime) > 60*60*24*182)//大于半年
	{
		p_date_format = "%b %e  %Y";
	}

	struct tm * p_tm = localtime(&local_time);
	strftime(datebuf, sizeof(datebuf), p_date_format, p_tm);//日期格式化
	
	return datebuf;
}
