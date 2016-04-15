#ifndef _STR_H_
#define _STR_H_

void str_trim_crlf(char *str);//去除字符串末尾的\r\n
void str_split(const char *str, char *left, char *right, char c);//按指定字符分割字符串
int str_all_space(const char *str);//判断字符串是否全为空格
void str_upper(char *str);//将字符串转换为大写
long long str_to_longlong(const char *str);//将字符串转换为long long
unsigned int str_octal_to_uint(const char *str);//将八进制表示字符串转换为无符号整数



#endif
