#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  

  int pipe1[2], pipe2[2];
  pipe(pipe1);
  pipe(pipe2);

  int pid=fork();
  if(pid!=0)
  {
    // parent
    close(pipe2[1]);
    write(pipe1[0], "a", 1);
    char s[1];
    read(pipe2[1], (void *)s, 1);
    int parent_id=getpid();
    printf("%d: received pong\n",parent_id);
    exit(0);
  }
  else
  {
    //child
    close(pipe1[1]);
    char s[1];
    read(pipe1[0], (void *)s, 1);
    int child_id=getpid();
    printf("%d: received ping\n", child_id);
    exit(0);
 
  }
}
