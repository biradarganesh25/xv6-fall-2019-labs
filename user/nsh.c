#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"
#include "kernel/fcntl.h"

#define exec_val 100
#define pipe_val 101
#define redirect_val 102

static int maxargs = 7;

static char whitespace[] = " \t\r\n\v";
static char symbols[] = "<|>&;()";




struct cmd
{
    int type;
};

struct execmd
{
    int type;
    char *argv[7];
    char *eargv[7];
    int ifd;
    int ofd;
    int imode;
    int omode;
    char *ifile;
    char *ofile;
    char *eifile;
    char *eofile;
};


struct pipecmd
{
    int type;
    struct execmd *left_cmd;
    struct execmd *right_cmd;
};


struct cmd * parsecmd(char *s, struct execmd *execcmd_inst1, struct execmd *execcmd_inst2, struct pipecmd *pipecmd_inst);
struct cmd *parsepipe(char **ps, char *es, struct execmd *execcmd_inst1, struct execmd *execcmd_inst2, struct pipecmd *pipecmd_inst);
struct cmd *parseexec(char **ps, char *es, struct execmd *execcmd_inst, struct pipecmd *pipecmd_inst);
void runcmd(struct cmd *final_cmd);
void driver(char *s);
int gettoken(char **ps, char *es, char **q, char **eq);
int peek(char **ps, char *es, char *toks);
void nullterminate(struct cmd * final_cmd);
int fork1(void);

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
  {
      fprintf(2,"fork failed!!");
      exit(-1);
  }
  return pid;
}

int gettoken(char **ps, char *es, char **q, char **eq)
{
    char *s;
    int ret;

    s = *ps;
    while (s < es && strchr(whitespace, *s))
        s++;
    if (q)
        *q = s;
    ret = *s;
    switch (*s)
    {
    case 0:
        break;
    case '|':
    case '(':
    case ')':
    case ';':
    case '&':
    case '<':
        s++;
        break;
    case '>':
        s++;
        if (*s == '>')
        {
            ret = '+';
            s++;
        }
        break;
    default:
        ret = 'a';
        while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
            s++;
        break;
    }
    if (eq)
        *eq = s;

    while (s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return ret;
}

int peek(char **ps, char *es, char *toks)
{
    char *s;

    s = *ps;
    while (s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return *s && strchr(toks, *s);
}

void runcmd(struct cmd *final_cmd)
{
    struct execmd *exec_final_cmd;
    struct pipecmd* pipe_final_cmd;

    switch (final_cmd->type)
    {
    case redirect_val:
    case exec_val:
        exec_final_cmd=(struct execmd *)final_cmd;
        nullterminate(final_cmd);

        // debug_statements
        // fprintf(2,"%s %s\n", exec_final_cmd->argv[0], exec_final_cmd->argv[1]);
        if(exec_final_cmd->ifd!=-1)
        {
            close(exec_final_cmd->ifd);
            open(exec_final_cmd->ifile,exec_final_cmd->imode);
        }
        if(exec_final_cmd->ofd!=-1)
        {
            close(exec_final_cmd->ofd);
            open(exec_final_cmd->ofile,exec_final_cmd->omode);
        }

        exec(exec_final_cmd->argv[0],exec_final_cmd->argv);
        break;
    case pipe_val:
        pipe_final_cmd=(struct pipecmd *)final_cmd;
        // printf("in pipe\n");
        // printf(":%s :%s :%s\n",pipe_final_cmd->left_cmd->argv[0],pipe_final_cmd->left_cmd->argv[1],pipe_final_cmd->right_cmd->argv[0]);
        // break;
        int p[2];
        int ret=pipe(p);
        if(ret <0)
        {
            fprintf(2,"pipe failed!!");
            exit(0);
        }
        if(fork1()==0)
        {
            close(1);
            dup(p[1]);
            close(p[0]);
            close(p[1]);
            runcmd((struct cmd *)pipe_final_cmd->left_cmd);
        }
        if(fork1()==0)
        {
            close(0);
            dup(p[0]);
            close(p[0]);
            close(p[1]);
            runcmd((struct cmd*)pipe_final_cmd->right_cmd);
        } 
        close(p[0]);
        close(p[1]);
        wait(0);
        wait(0);
        // printf("%s %s\n",pipe_final_cmd->left_cmd->argv[1],pipe_final_cmd->right_cmd->argv[1]);
        break;

    
    default:
        break;
    }
}

void nullterminate(struct cmd * final_cmd)
{
    struct execmd *exec_final_cmd;
    switch (final_cmd->type)
    {
    case redirect_val:
    case exec_val:
        exec_final_cmd=(struct execmd *)final_cmd;

        int i=0;
        while(exec_final_cmd->argv[i]!=0)
        {
            *exec_final_cmd->eargv[i]=0;
            i++;
        }
        if(exec_final_cmd->ifd!=-1)
        *exec_final_cmd->eifile=0;
        if(exec_final_cmd->ofd!=-1)
        *exec_final_cmd->eofile=0;
        break;

    case pipe_val:
        fprintf(2,"Shouldn't have come here in pipe!\n");
        exit(1);

    default:
        fprintf(2,"Shouldn't have come here!!\n");
        exit(1);
        break;
    }
}
void driver(char *s)
{
    struct execmd execcmd_inst1, execcmd_inst2;
    struct pipecmd pipecmd_inst;

    execcmd_inst1.type=exec_val;
    execcmd_inst2.type=exec_val;
    pipecmd_inst.type=pipe_val;

    execcmd_inst1.ifd=-1;
    execcmd_inst1.ofd=-1;
    execcmd_inst2.ifd=-1;
    execcmd_inst2.ofd=-1;
    

    struct cmd *final_cmd=parsecmd(s,&execcmd_inst1,&execcmd_inst2,&pipecmd_inst);

    if(final_cmd==0)
    {
        fprintf(2,"Something wrong in parsing cmd\n");
        exit(0);
    }

    // nullterminate(final_cmd);

    runcmd(final_cmd);
}
struct cmd *parsecmd(char *s, struct execmd *execcmd_inst1, struct execmd *execcmd_inst2, struct pipecmd *pipecmd_inst)
{
    char *es = s + strlen(s);
    struct cmd *final_cmd = parsepipe(&s, es, execcmd_inst1, execcmd_inst2, pipecmd_inst);
    return final_cmd;
}

struct cmd *parsepipe(char **ps, char *es, struct execmd *execcmd_inst1, struct execmd *execcmd_inst2, struct pipecmd *pipecmd_inst)
{
    struct cmd *final_cmd = parseexec(ps, es, execcmd_inst1, pipecmd_inst);
    char *q, *eq;
    if (peek(ps, es, "|"))
    {
        gettoken(ps, es, &q, &eq);
        pipecmd_inst->left_cmd = (struct execmd *)final_cmd;
        final_cmd = parseexec(ps, es, execcmd_inst2, pipecmd_inst);
        pipecmd_inst->right_cmd = (struct execmd *)final_cmd;
        return (struct cmd *)pipecmd_inst;
    }

    return final_cmd;
}

struct cmd *parseexec(char **ps, char *es, struct execmd *execcmd_inst, struct pipecmd *pipecmd_inst)
{
    int ret;
    char *q, *eq;
    int i = 0;

    while (*(*ps) != 0)
    {
        if (i > maxargs-1)
        {
            fprintf(2, "More than %d args\n", maxargs);
            exit(0);
        }
        if (peek(ps, es, "|"))
        {
            execcmd_inst->argv[i] = 0;
            return (struct cmd *)execcmd_inst;
        }

        if (peek(ps, es, ">"))
        {
            execcmd_inst->ofd = 1;
            ret = gettoken(ps, es, &q, &eq);
            ret = gettoken(ps, es, &q, &eq);
            if(ret!='a')
            {
                fprintf(2, "Wrong syntax in >\n");
                exit(0);
            }
            execcmd_inst->ofile = q;
            execcmd_inst->eofile = eq;
            execcmd_inst->omode = (O_CREATE | O_WRONLY);
        }
        else if (peek(ps, es, "<"))
        {
            execcmd_inst->ifd = 0;
            ret = gettoken(ps, es, &q, &eq);
            ret = gettoken(ps, es, &q, &eq);
            if(ret!='a')
            {
                fprintf(2, "Wrong syntax in <\n");
                exit(0);
            }
            execcmd_inst->ifile = q;
            execcmd_inst->eifile = eq;
            execcmd_inst->imode = O_RDONLY;
        }
        else
        {
            ret = gettoken(ps, es, &q, &eq);
            if (ret != 'a')
            {
                fprintf(2, "Wrong syntax in main loop, ret: %d\n", ret);
                exit(0);
            }
            execcmd_inst->argv[i] = q;
            execcmd_inst->eargv[i] = eq;
            // printf("q:%s, eq:%s\n", q, eq);
            i++;
        }

    }
    execcmd_inst->argv[i]=0;

    return (struct cmd *)execcmd_inst;
}

int main(void)
{
    char buf[100];

    printf("@ ");
    while(strlen(gets(buf,99))!=0)
    {
        if(fork1()==0)
        driver(buf);
        wait(0);
        printf("@ ");
    }
    exit(0);
}