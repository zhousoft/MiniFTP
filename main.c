#include "common.h"
int main()
{
	if(getuid() != 0)//如果不是root用户启动
	{
		fprintf(stderr,"miniftpd: must be started ad root\n");
		exit(EXIT_FAILURE);
	}
	return 0;
}
