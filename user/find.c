#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

static int max_file_len=512;

int recur(char *cur_dir, char *to_find)
{
  int fd;
  struct stat st;
  struct dirent de;

  if ((fd = open(cur_dir, 0)) < 0)
  {
    fprintf(2, "find: cannot open %s\n", cur_dir);
    return -1;
  }

  if (fstat(fd, &st) < 0)
  {
    fprintf(2, "find: cannot stat %s\n", cur_dir);
    close(fd);
    return -1;
  }
  char *p = cur_dir + strlen(cur_dir);
  *p++ = '/' ;
  while (read(fd, &de, sizeof(de)) == sizeof(de))
  {
    if (de.inum == 0)
      continue;

    if(strcmp(de.name,".")==0 || strcmp(de.name,"..")==0)
    {
      continue;
    }
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;

    if (stat(cur_dir, &st) < 0)
    {
      fprintf(2, "find: cannot stat %s\n", cur_dir);
      close(fd);
      return -1;
    }
    switch (st.type)
    {
    case T_DEVICE:
    case T_FILE:
      if (strcmp(to_find, p)==0)
      {
        printf("%s\n",cur_dir);
      }
      break;
    case T_DIR:
      if(strlen(cur_dir)+DIRSIZ+1>max_file_len)
      {
        fprintf(2,"Max file name length> 512");
        return -1;
      }
      recur(cur_dir, to_find);
      break;
    default:
      printf("Neither file nor dir!: %s hmm.. \n",cur_dir);
      break;
    }
  }
  close(fd);

  return -1;
}


int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("Usage: find dir name");
    exit(0);
  }

  char *to_find = argv[2];
  char cur_dir[513];
  strcpy(cur_dir, argv[1]);
  // todo: remove trailing slashes.
  recur(cur_dir, to_find);
  exit(0);
  
}