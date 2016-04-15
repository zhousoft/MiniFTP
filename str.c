#include "common.h"
#include "str.h"

void str_trim_crlf(char *str)
{
	char *p = &str[strlen(str)-1];//指向最后一个字符
	while(*p == '\r' || *p == '\n')
		*p-- = '\0';
}

void str_split(const char *str, char *left, char *right, char c)
{
	char *p = strchr(str,c);//查找字符串中首次出现c的位置
	if(p == NULL)//没有找到，字符串不包含c
	{
		strcpy(left,str);
	}
	else
	{
		strncpy(left, str, p-str);
		strcpy(right, p+1);
	}
}

int str_all_space(const char *str)
{
	while(*str)
	{
		if(!isspace(*str))
			return 0;
		str++;
	}
	return 1;
}

void str_upper(char *str)
{
	while(*str)
	{
		*str = toupper(*str);
		str++;
	}
}

long long str_to_longlong(const char *str)
{
	//return atoll(str);
	long long result = 0;
	long long mult = 1;
	unsigned int len = strlen(str);
    int i;
	if(len > 15)
		return 0;
	
	for(i=len-1; i>=0; i--)
	{
		char ch = str[i];
		long long val;
		if(ch < '0' || ch >'9')
			return 0;
		
		val = ch - '0';
		val *= mult;
		result += val;
		mult *= 10;
	}
	return result;
}

unsigned int str_octal_to_uint(const char *str)
{
	unsigned int result = 0;
	int seen_non_zero_digit = 0;

	while(*str)
	{
		int digit = *str;
		if(!isdigit(digit) || digit > '7')
			break;
		if(digit != '0')
			seen_non_zero_digit = 1;

		if(seen_non_zero_digit)
		{
			result <<= 3;//乘以8
			result += (digit - '0');
		}
		str++;
	}

	return result;
}
