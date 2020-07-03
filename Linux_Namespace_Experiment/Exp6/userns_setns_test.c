/* userns_setns_test.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Open a /proc/PID/ns/user namespace file specified on the command
   line, and then create a child process in a new user namespace.
   Both processes then try to setns() into the namespace identified
   on the command line.  The setns() system call requires
   CAP_SYS_ADMIN in the target namespace.
*/
#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define errExit(msg)                    \
    do                                  \
    {                                   \
        perror(msg);                    \
        exit(EXIT_FAILURE);             \
    } while (0)

static void test_setns(char *pname, int fd)
{
    char path[PATH_MAX] = {'\0'};
    ssize_t s = 0;

    s = readlink("/proc/self/ns/user", path, PATH_MAX);
    if(-1 == s)
    {
        errExit("readlink");
    }

    printf("%s readlink(\"/proc/self/ns/user\") ==> %s\n", pname, path);

    if(-1 == setns(fd, CLONE_NEWUSER))
    {
        printf("%s setns() failed: %s\n", pname, strerror(errno));
    }
    else
    {
        printf("%s setns() succeeded\n", pname);
    }
}

static int childFunc(void *arg)
{
    long fd = (long) arg;

    usleep(100000);

    test_setns("child", fd);

    return 0;
}

#define STACK_SIZE (1024 * 1024)
static char child_stack[STACK_SIZE]; 

int main(int argc, char* argv[])
{
    pid_t child_pid;
    long fd = 0;

    memset(&child_pid, 0, sizeof(pid_t));

    if(argc <2)
    {
        fprintf(stderr, "Usage: %s /proc/PID/ns/user]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if(-1 == (fd = open(argv[1], O_RDONLY)))
    {
        errExit("open");
    }

    child_pid = clone(childFunc, child_stack + STACK_SIZE,
                        CLONE_NEWUSER | SIGCHLD, (void *)fd);
    if(-1 == child_pid)
    {
        errExit("clone");
    }

    test_setns("parent:", fd);
    printf("\n");

    if(-1 == waitpid(child_pid, NULL, 0))
    {
        errExit("waitpid");
    }

     exit(EXIT_SUCCESS);
}


