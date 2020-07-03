/* ns_child_exec.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Create a child process that executes a shell command in new namespace(s).
*/

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#define errExit(msg)            \
    do                          \
    {                           \
        perror(msg);            \
        exit(EXIT_FAILURE);     \
    }while(0)
        
static void usage(char *pname)
{
    fprintf(stderr, "Usage: %s [options] cmd [arg...]\n", pname);
    fprintf(stderr, "Options can be\n");
    fprintf(stderr, "   -i  new IPC namepaces\n");
    fprintf(stderr, "   -m  new mount namepaces\n");
    fprintf(stderr, "   -n  new network namepaces\n");
    fprintf(stderr, "   -p  new PID namepaces\n");
    fprintf(stderr, "   -u  new UTS namepaces\n");
    fprintf(stderr, "   -U  new user namepaces\n");
    fprintf(stderr, "   -v  Display verbose message\n");
    exit(EXIT_FAILURE);
}

static int childFunc(void *arg)
{
    char **argv = arg;

    execvp(argv[0], &argv[0]);
    errExit("execvp");
}

#define STACK_SIZE (1024 * 1024)
static char child_stack[STACK_SIZE];

int main(int argc, char* argv[])
{
    int flags = 0;
    int opt = 0;
    int verbose = 0;
    pid_t child_pid;
    
    memset(&child_pid, 0, sizeof(child_pid));
    
    while(-1 != (opt = getopt(argc, argv, "+imnpuUv")))
    {
        switch(opt)
        {
            case 'i' : flags |= CLONE_NEWIPC;  break;
            case 'm' : flags |= CLONE_NEWNS;   break;
            case 'n' : flags |= CLONE_NEWNET;  break;
            case 'p' : flags |= CLONE_NEWPID;  break;
            case 'u' : flags |= CLONE_NEWUTS;  break;
            case 'U' : flags |= CLONE_NEWUSER; break;
            case 'v' : verbose = 1; break;
            default: usage(argv[0]);
        }
    }
    
    if(-1 == (child_pid = clone(childFunc, child_stack + STACK_SIZE, flags | SIGCHLD, &argv[optind])))
    {
        errExit("clone failed.");
    }
    
    if(verbose)
    {
        printf("%s: PID of child created by clone() is %ld\n", argv[0], (long) child_pid);
    }
    
    if(-1 == waitpid(child_pid, NULL, 0))
    {
        errExit("waitpid failed.");
    }
    
    if(verbose)
    {
        printf("%s: terminating\n", argv[0]);
    }
    
    exit(EXIT_SUCCESS);
}







