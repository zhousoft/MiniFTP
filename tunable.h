#ifndef _TUNABLE_H_
#define _TUNABLE_H_



extern int tunable_pasv_enable;//被动模式使能变量
extern int tunable_port_enable;//主动模式使能变量
extern unsigned int tunable_listen_port;//FTP服务器端口
extern unsigned int tunable_max_clients;//最大连接客户端数
extern unsigned int tunable_max_per_ip;//每IP最大连接数
extern unsigned int tunable_accept_timeout;//accept连接超时时间
extern unsigned int tunable_connect_timeout;//connect连接超时时间
extern unsigned int tunable_idle_session_timeout;//控制连接超时时间
extern unsigned int tunable_data_connection_timeout;//数据连接超时时间
extern unsigned int tunable_local_umask;//掩码
extern unsigned int tunable_upload_max_rate;//最大上传速度
extern unsigned int tunable_download_max_rate;//最大下载速度
extern const char *tunable_listen_address;//FTP服务器地址


#endif
