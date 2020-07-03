/* ns_exec.c

Copyright 2013, Michael Kerrisk
Licensed under GNU General Public License v2 or later

Join a namespace and execute a command in the namespace
*/

#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define errExit(msg)        \
    do                      \
    {                       \
        perror(msg);        \
        exit(EXIT_FAILURE); \
    } while (0)

int main(int argc, char *argv[])
{
    int fd = 0;

    if (argc < 3)
    {
        fprintf(stderr, "%s /proc/PID/ns/FILE cmd [arg...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fd = open(argv[1], O_RDONLY);
    if (-1 == fd)
    {
        errExit("open failed.");
    }

    if (-1 == setns(fd, 0))
    {
        errExit("setns failed.");
    }

    execvp(argv[2], &argv[2]);
    errExit("setns failed.");
}