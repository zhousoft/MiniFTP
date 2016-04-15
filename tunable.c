#include "tunable.h"

int tunable_pasv_enable = 1;//被动模式使能变量
int tunable_port_enable = 1;//主动模式使能变量
unsigned int tunable_listen_port = 21;//服务器端口
unsigned int tunable_max_clients = 2000;//最大连接客户端数
unsigned int tunable_max_per_ip = 50;
unsigned int tunable_accept_timeout = 60;//接收连接阻塞时间
unsigned int tunable_connect_timeout = 60;//连接阻塞时间
unsigned int tunable_idle_session_timeout = 300;
unsigned int tunable_data_connection_timeout = 300;
unsigned int tunable_local_umask = 077;
unsigned int tunable_upload_max_rate = 0;
unsigned int tunable_download_max_rate = 0;
const char *tunable_listen_address;
