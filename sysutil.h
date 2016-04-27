#ifndef _SYS_UTIL_H_
#define _SYS_UTIL_H_

#include "common.h"/*整合的通用头文件*/

int tcp_client(unsigned short port);
int tcp_server(const char *host, unsigned short port); 

int getlocalip(char *ip);/*获取本地ip*/

void activate_nonblock(int fd);/*激活非阻塞模式*/
void deactivate_nonblock(int fd);/*取消非阻塞模式*/

int read_timeout(int fd, unsigned int wait_seconds);/*读取超时时间*/
int write_timeout(int fd, unsigned int wait_seconds);/*写超时*/
int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);/*接收连接超时*/
int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);/*连接超时*/

ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd,const void *buf, size_t count);
ssize_t recv_peek(int sockfd, void *buf, size_t len);/*窥探socket缓冲中是否有数据，不清除*/
ssize_t readline(int sockfd, void *buf, size_t maxline);

void send_fd(int sock_fd, int fd);/*发送文件描述符*/
int  recv_fd(const int sock_fd);/*接收文件描述符*/

const char *statbuf_get_perms(struct stat *sbuf);//封装的获取文件权限函数
const char *statbuf_get_date(struct stat *sbuf);//获取文件日期函数
#endif/*_SYS_UTIL_H_*/
