#ifndef _COMMON_H_
#define _COMMON_H_

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>


#define ERR_EXIT(m)\
	do\
	{\
		perror(m);\
		exit(EXIT_FAILURE);\
	}\
	while(0)


#define MAX_COMMAND_LINE 1024 //客户端发送的一行命令最大长度
#define MAX_COMMAND 32
#define MAX_ARG 1024 //参数最大长度
#define MINIFTP_CONF "miniftpd.conf"//配置文件路径


#endif /*_COMMON_H_*/
