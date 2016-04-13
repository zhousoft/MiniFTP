#ifndef _SYS_UTIL_H_
#define _SYS_UTIL_H_

#include "common.h"/*整合的通用头文件*/

#define ERR_EXIT(m)\
	do\
	{\
		perror(m);\
		exit(EXIT_FAILURE);\
	}\
	whie(0)

int getlocalip(char *ip);/*换取本地ip*/

void activate_nonblock(int fd);/*激活非阻塞模式*/
void deactivate_nonblock(int fd);/*取消非阻塞模式*/

int read_timeout(int fd, unsigned int wait_seconds);/*读取超时时间*/
int write_timeout(int fd, unsigned int wait_seconds);/*写超时*/
int accept_timeout(int fd, struct socktaddr_in *addr, unsigned int wait_seconds);/*接收连接超时*/
int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);/*连接超时*/

ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd,const void *buf, size_t count);
ssize_t recv_peek(int sockfd, void *buf, size_t len);
ssize_t readline(int sockfd, void *buf, size_t maxline);

void send_fd(int sock_fd, int fd);/*发送文件描述符*/
void recv_fd(const int sock_fd);/*接收文件描述符*/

#endif/*_SYS_UTIL_H_*/
