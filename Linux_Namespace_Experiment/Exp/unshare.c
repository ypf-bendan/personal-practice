/* unshare.c

Copyright 2013, Michael Kerrisk
Licensed under GNU General Public License v2 or later

A simple implementation of the unshare(1) command: unshare
namespaces and execute a command.
*/

#define _GNU_SOURCE
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

static void usage(char *pname)
{
    if (NULL == pname)
    {
        return;
    }
    fprintf(stderr, "Usage: %s [options] program [arg...]\n", pname);
    fprintf(stderr, "Options can be:\n");
    fprintf(stderr, "    -i unshare IPC namespace\n");
    fprintf(stderr, "    -m unshare mount namespace\n");
    fprintf(stderr, "    -n unshare network namespace\n");
    fprintf(stderr, "    -p unshare PID namespace\n");
    fprintf(stderr, "    -u unshare UTS namespace\n");
    fprintf(stderr, "    -U unshare user namespace\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    extern char *optarg;
    extern int optind, opterr, optopt;

    int flags = 0;
    int opt   = 0;

    while (-1 != (opt = getopt(argc, argv, "imnpuU")))
    {
        switch (opt)
        {
        case 'i': flags |= CLONE_NEWIPC;
            break;
        case 'm': flags |= CLONE_NEWNS;
            printf("run");
            break;
        case 'n': flags |= CLONE_NEWNET;
            break;
        case 'p': flags |= CLONE_NEWPID;
            break;
        case 'u': flags |= CLONE_NEWUTS;
            break;
        case 'U': flags |= CLONE_NEWUSER;
            break;
        default: usage(argv[0]);
        }
    }

    if (optind >= argc)
    {
        usage(argv[0]);
    }

    if (-1 == unshare(flags))
    {
        usage(argv[0]);
    }

    execvp(argv[optind], &argv[optind]);
    return 0;
}