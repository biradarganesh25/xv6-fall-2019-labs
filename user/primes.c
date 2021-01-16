#include "kernel/types.h"
#include "user/user.h"


void child(int rfd)
{
	int num=-1,start=-1;

	int bytes;
	int p[2];
	pipe(p);
	bytes = read(rfd, (void *)&num, 4);
	while(bytes > 0)
	{
		if(start == -1)
		{
			start = num;
			num = -1;
			printf("prime %d\n",start);
		}
		else if(num % start != 0)
		{
			write(p[1], (void *)&num, 4);
		}
		bytes = read(rfd, (void *)&num, 4);
	}

	close(rfd);
	close(p[1]);
	if(num != -1)
	{
		child(p[0]);
		wait(0);
	}
	else
	{
		close(p[0]);
	}
	exit(0);

}
int main(int argc, char *argv[])
{
	int p[2];
	pipe(p);

	int i;
	for(i=2;i<=35;i++)
	{
		write(p[1], (void *)&i, 4);
	}

	close(p[1]);

	child(p[0]);
	wait(0);
	exit(0);
}
