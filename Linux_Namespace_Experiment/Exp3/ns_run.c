/* ns_run.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Join one or more namespaces using setns() and execute a command in
   those namespaces, possibly inside a child process.

   This program is similar in concept to nsenter(1), but has a
   different command-line interface.
*/
#define _GNU_SOURCE
#include <fcntl.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

#define errExit(msg)            \
    do                          \
    {                           \
        perror(msg);            \
        exit(EXIT_FAILURE);     \
    }while(0)

static void usage(char* pname)
{
    fprintf(stderr, "Usage: %s [-f] [-n /proc/PID/ns/FILE] cmd [arg...]\n", pname);
    fprintf(stderr, "\t-f   Execute command in child process\n");
    fprintf(stderr, "\t-f   Join specified namespace\n");
    
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
    int fd = 0, opt = 0, do_fork = 0;
    pid_t pid;
    
    memset(&pid, 0, sizeof(pid_t));
    while(-1 != (opt = getopt(argc, argv, "+fn:")))
    {
        switch(opt)
        {
            case 'f': do_fork = 1; 
                      break;
            case 'n': fd = open(optarg, O_RDONLY);
                      if(-1 == fd)
                      {
                          errExit("open");
                      }
                      
                      if(-1 == setns(fd, 0))
                      {
                          errExit("setns");
                      }
                      break;
            default:  usage(argv[0]);
        }
    }
    
    if(argc <= optind)
    {
        usage(argv[0]);
    }
    
    if(do_fork)
    {
        pid = fork();
        if(-1 == pid)
        {
            errExit("fork");
        }
        
        if(0 != pid)
        {
            if(-1 == waitpid(-1, NULL, 0))
            {
                errExit("waitpid");
            }
            exit(EXIT_SUCCESS);
        }
    }

    printf("%s\n", argv[optind]);
    execvp(argv[optind], &argv[optind]);
    errExit("execvp");
}