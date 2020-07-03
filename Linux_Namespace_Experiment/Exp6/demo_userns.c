/* demo_userns.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Demonstrate the use of the clone() CLONE_NEWUSER flag.

   Link with "-lcap" and make sure that the "libcap-devel" (or
   similar) package is installed on the system.
*/
#define _GNU_SOURCE
#include <sys/capability.h>
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define errExit(msg)          \
    do                        \
    {                         \
        perror(msg);          \
        exit(EXIT_FAILURE);   \
    }while(0)

static int childFunc(void* arg)
{
    cap_t caps;
    memset(&caps, 0, sizeof(caps));

    for(;;)
    {
        printf("eUID = %ld; eGID = %ld; ", 
               (long) geteuid(), (long)getegid());
	caps = cap_get_proc();
	printf("capabilities: %s\n", cap_to_text(caps, NULL));

	if(NULL == arg)
        {
	    break;
        }
        sleep(5);
    }
    return 0;
}

#define STACK_SIZE (1024*1024)
static char child_stack[STACK_SIZE];

int main(int argc, char* argv[])
{
    pid_t pid;
    memset(&pid, 0, sizeof(pid_t));

    pid = clone(childFunc, child_stack + STACK_SIZE,
                CLONE_NEWUSER | SIGCHLD, argv[1]);
    if(-1 == pid)
    {
        errExit("clone");
    }
    
    if(-1 == waitpid(pid, NULL, 0))
    {
        errExit("waitpid");
    }
    exit(EXIT_SUCCESS);
}
