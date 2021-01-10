#include "kernel/types.h"
#include "user/user.h"
int
main(int argc, char *argv[])
{
	if(argc != 2)
	{
		printf("Usage: sleep seconds\n");
		exit(0);
	}

	int seconds=atoi(argv[1])*10;
	printf("Seconds is %d\n",seconds);
	sleep(seconds);
	exit(0);
}

