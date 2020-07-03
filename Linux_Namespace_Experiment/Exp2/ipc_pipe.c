#define _GNU_SOURCE
#include <sys/capability.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define errExit(msg)            \
    do                          \
    {                           \
        perror(msg);            \
        exit(EXIT_FAILURE);     \
    }while(0)

#define STACK_SIZE  (1024 * 1024)

int checkpoint[2]; 
static char child_stack[STACK_SIZE]; 
char* const child_args[] = { "/bin/bash", NULL }; 

int child_main(void* arg) 
{
    int idx = 5;
    char c[15] = "I am child!"; 
    close(checkpoint[0]);

    while(idx--)
    {
        write(checkpoint[1], c, sizeof(c));
        sleep(2);
    }
    close(checkpoint[1]);

    printf(" - World !\n"); 
    sethostname("NewNamespace", 12); 
    execv(child_args[0], child_args); 
    printf("Ooops\n"); 
    return 1; 
} 
int main(int argc, char* argv[]) 
{
    int idx = 5;
    int ret = 0;
    int child_pid = 0;
    int status = 0;
    char msg[15] = {'\0'};

    ret = pipe(checkpoint);
    if(-1 == ret)
    {
        errExit("pipe failed.");
    }

    printf(" - Hello ?\n"); 
    child_pid = clone(child_main, child_stack + STACK_SIZE, CLONE_NEWUTS | CLONE_NEWIPC | SIGCHLD, NULL); 
    if(-1 == child_pid)
    {
        errExit("clone failed.");
    }

    sleep(4);

    close(checkpoint[1]); 
    while(idx--)
    {
        memset(msg, 0, sizeof(msg));
        ssize_t s = read(checkpoint[0], msg, sizeof(msg));
        if(s > 0)
        {
            msg[s - 1] = '\0';
        }
        printf("%s, %d\n", msg, idx);
    }
    close(checkpoint[0]);
    ret = waitpid(child_pid, &status, 0); 
    if(-1 == ret)
    {
        errExit("waitpid failed.\n");
    }
    
    printf("exitsingle(%d),exit(%d)\n",status&0xff,(status>>8)&0xff);

    exit(EXIT_SUCCESS);
}