/* multi_pidns.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Create a series of child processes in nested PID namespaces.
*/

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>

#define errExit(msg)            \
    do                          \
    {                           \
        perror(msg);            \
        exit(EXIT_FAILURE);     \
    }while(0)

#define STACK_SIZE (1024 * 1024)
static char child_stack[STACK_SIZE];

static int childFunc(void* arg)
{
    static int first_call = 1;
    long level = (long) arg;
    
    if(! first_call)
    {
        char mount_point[PATH_MAX] = {'\0'};
        
        snprintf(mount_point, PATH_MAX, "/proc%c", (char) ('0' + level));
        mkdir(mount_point, 0555);
        if(-1 == mount("proc", mount_point, "proc", 0, NULL))
        {
            errExit("mount");
        }
        
        printf("Mounting procfs at %s\n", mount_point);
    }
    
    first_call = 0;
    if(0 < level)
    {
        level--;
        pid_t child_pid;
        memset(&child_pid, 0, sizeof(pid_t));
        
        child_pid = clone(childFunc, child_stack + STACK_SIZE,
                            CLONE_NEWPID | SIGCHLD, (void*)level);
        if(-1 == child_pid)
        {
            errExit("clone");
        }
        
        if(-1 == waitpid(child_pid, NULL, 0))
        {
            errExit("waitpid");
        }
    }
    else
    {
        printf("Final child sleeping\n");
        execlp("sleep", "sleep", "1000", (char*) NULL);
        errExit("execlp");
    }
    
    return 0;
}

int main(int argc, char* argv[])
{
    long levels = 0;
    
    levels = (argc > 1) ? atoi(argv[1]) : 5;
    childFunc((void*)levels);
    
    exit(EXIT_SUCCESS);
}




















