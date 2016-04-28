#include "parseconf.h"
#include "common.h"
#include "tunable.h"
#include "str.h"
//bool类型的配置项
static struct parseconf_bool_setting
{
	const char *p_setting_name;
	int *p_variable;
}
parseconf_bool_array[] = 
{
	{"pasv_enable", &tunable_pasv_enable},
	{"port_enable", &tunable_port_enable},
	{NULL, NULL}//作为遍历时的结束判断
};

//uint类型的配置项
static struct parseconf_uint_setting
{
	const char *p_setting_name;
	unsigned int *p_variable;
}
parseconf_uint_array[] =
{
	{"listen_port", &tunable_listen_port},
	{"max_clients", &tunable_max_clients},
	{"max_per_ip", &tunable_max_per_ip},
	{"accept_timeout", &tunable_accept_timeout},
	{"connect_timeout", &tunable_connect_timeout},
	{"idle_session_timeout", &tunable_idle_session_timeout},
	{"data_connection_timeout", &tunable_data_connection_timeout},
	{"local_umask", &tunable_local_umask},
	{"upload_max_rate", &tunable_upload_max_rate},
	{"download_max_rate", &tunable_download_max_rate},
	{NULL, NULL}
};

//字符串类型的配置项
static struct parseconf_str_setting
{
	const char *p_setting_name;
	const char **p_variable;
}
parseconf_str_array[] = 
{
	{"listen_address", &tunable_listen_address},
	{NULL, NULL}
};


void parseconf_load_file(const char *path)
{
	FILE *fp = fopen(path, "r");
	if(fp == NULL)
	ERR_EXIT("foprn");

	char setting_line[1024] = {0};
	while(fgets(setting_line, sizeof(setting_line), fp) != NULL)//按行读取配置
	{
		if(strlen(setting_line) == 0
			|| setting_line[0] == '#'
			|| str_all_space(setting_line))//读取行长度为空|第一个字符为注释# |读取行全为空白字符
			continue;

			str_trim_crlf(setting_line);//去除读取到的行尾的\r\n
			parseconf_load_setting(setting_line);//加载配置
			memset(setting_line,0,sizeof(setting_line));
	}
	fclose(fp);
}


void parseconf_load_setting(const char *setting)
{
	while(isspace(*setting))//去除左部空格
		setting++;
	char key[128] = {0};
	char value[128] = {0};
	str_split(setting, key, value, '=');//解析出配置项名称和配置值
	if(strlen(value) == 0)//配置项中没有等号或者等号后为空
	{
		if(strcmp(key, "listen_address") != 0)//只有ip地址允许为空-监听本机随机ip
		{
			fprintf(stderr, "miss value in config file for: %s\n", key);
			exit(EXIT_FAILURE);
		}
	}
	//根据配置项的key在三种配置类型表格中搜索对应的配置项
	//1.查找字符串类型的配置项
	const struct parseconf_str_setting *p_str_setting = parseconf_str_array;
	while(p_str_setting->p_setting_name != NULL)
	{
		if(strcmp(key, p_str_setting->p_setting_name) == 0)//找到匹配项
		{
			const char **p_cur_setting = p_str_setting->p_variable;
			if(*p_cur_setting)//清除原先设置，释放内存
				free((char*)*p_cur_setting);

			*p_cur_setting = strdup(value);//利用strdup开辟空间并拷贝字符串
			return;

		}
		p_str_setting++;
	}
	
	//2.查找bool类型的配置项
	const struct parseconf_bool_setting *p_bool_setting = parseconf_bool_array;
	while(p_bool_setting->p_setting_name != NULL)
	{
		if(strcmp(key,p_bool_setting->p_setting_name) == 0)
		{
			str_upper(value);//统一转换为大写，便于判断
			if(strcmp(value, "YES") == 0 || strcmp(value, "TRUE") == 0 || strcmp(value, "1") == 0)
				*(p_bool_setting->p_variable) = 1;
			else if(strcmp(value, "NO") == 0 || strcmp(value, "FALSE") == 0 || strcmp(value, "0") == 0)
					*(p_bool_setting->p_variable) = 0;
				 else
				 {
					fprintf(stderr, "bad bool value in config file for: %s\n", key);
		            exit(EXIT_FAILURE);
				 }
			return;
		}
		p_bool_setting++;
	}

	//*.查找uint类型的配置项
	const struct parseconf_uint_setting *p_uint_setting = parseconf_uint_array;
	while(p_uint_setting->p_setting_name != NULL)
	{
		if(strcmp(key, p_uint_setting->p_setting_name) == 0)
		{
			if(value[0] == '0')//设置数值为8进制
				*(p_uint_setting->p_variable) = str_octal_to_uint(value);
			else
				*(p_uint_setting->p_variable) = atoi(value);
			return;
		}
		p_uint_setting++;
	}


	


}
