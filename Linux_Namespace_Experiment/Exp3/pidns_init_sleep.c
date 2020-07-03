/* pidns_init_sleep.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   A simple demonstration of PID namespaces.
*/

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#define errExit(msg)    	\
    do				\
    {				\
        perror(msg);		\
        exit(EXIT_FAILURE);	\
    }while(0)


static int childFunc(void *arg)
{
    printf("childFunc() : PID = %ld\n", (long) getpid());
    printf("childFunc() : PPID = %ld\n", (long) getppid());
    
    char *mount_point = arg;

    if(NULL != mount_point)
    {
        mkdir(mount_point, 0555);
        if(-1 == mount("proc", mount_point, "proc", 0, NULL))
        {
            errExit("mount failed.");
        }
        
        printf("Mounting procfs at %s\n", mount_point);
    }
    
    execlp("sleep", "sleep", "600", (char *)NULL);
    errExit("execlp");
}

#define STACK_SIZE (int)(1024 * 1024)
static char child_stack[STACK_SIZE];

int main(int argc, char *argv[])
{
    pid_t child_pid;
    child_pid = clone(childFunc, child_stack + STACK_SIZE, CLONE_NEWPID | SIGCHLD, argv[1]);
    if(-1 == child_pid)
    {
        errExit("clone failed.");
    }
    
    printf("PID returned by clone(): %ld\n", (long) child_pid);
    
    if(-1 == waitpid(child_pid, NULL, 0))
    {
        errExit("waitpid failed.");
    }
    
    exit(EXIT_SUCCESS);
}
