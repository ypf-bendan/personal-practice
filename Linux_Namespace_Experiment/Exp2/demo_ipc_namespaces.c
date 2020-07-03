/* demo_uts_namespaces.c

Copyright 2013, Michael Kerrisk
Licensed under GNU General Public License v2 or later

Demonstrate the operation of UTS namespaces.
*/

#define _GNU_SOURCE
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sched.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define STACK_SIZE (int)(1024 * 1024)

/*
 *增加一个错误打印的宏
*/
#define errExit(msg)        \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

static char child_stack[STACK_SIZE];

char* const child_args[] = {
  "/bin/bash",
  NULL
};

/*clone()函数调用函数*/
static int childFunc(void *arg)
{
    struct utsname uts = { { 0 } };

    if (-1 == sethostname(arg, strlen((char *)arg)))
    {
        errExit("sethostname failed.");
    }

    if (-1 == uname(&uts))
    {
        errExit("uname failed.");
    }

    printf("uts.nodename in child: %s.\n", uts.nodename);

    /*
        *保持命令空间是开放的状态，可以允许其他进程加入到这个命名空间
    */
    //sleep(10);
    execv(child_args[0], child_args);

    return 0;
}


int main(int argc, char *argv[])
{
    pid_t child_pid;
    struct utsname uts;

    memset(&child_pid, 0, sizeof(pid_t));
    memset(&uts, 0, sizeof(struct utsname));

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <child-hostname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /*因为栈是反着的，所以加上STACK_SIZE*/
    child_pid = clone(childFunc, child_stack + STACK_SIZE,
        CLONE_NEWUTS | CLONE_NEWIPC | SIGCHLD, argv[1]);
    if (-1 == (long)child_pid)
    {
        errExit("clone failed.");
    }
    printf("PID of child created by clone() is %ld\n", (long)child_pid);

    /*
        *父进程休眠1s， 让子进程有时间去修改hostname
    */
    sleep(1);

    if (-1 == uname(&uts))
    {
        errExit("uname failed.");
    }
    printf("uts.nodename in parent: %s\n", uts.nodename);

    if (-1 == waitpid(child_pid, NULL, 0))
    {
        errExit("waitpid failed.");
    }
    printf("child has terminated\n");

    return 0;
}