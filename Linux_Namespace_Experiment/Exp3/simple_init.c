/* simple_init.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   A simple init(1)-style program to be used as the init program in
   a PID namespace. The program reaps the status of its children and
   provides a simple shell facility for executing commands.
*/

#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <wordexp.h>
#include <errno.h>
#include <sys/wait.h>

#define errExit(msg)            \
    do                          \
    {                           \
        perror(msg);            \
        exit(EXIT_FAILURE);     \
    }while(0)
        
static int verbose = 0;
static void child_handler(int sig)
{
    pid_t pid;
    int status = 0;
    
    memset(&pid, 0, sizeof(pid_t));
    
    while(0 != (pid = waitpid(-1, &status, 
                              WNOHANG | WUNTRACED | WCONTINUED)))
    {
        if(-1 == pid)
        {
            if(ECHILD == errno)
            {
                break;
            }
            else
            {
                perror("waitpid");
            }
        }
        
        if(verbose)
        {
            printf("\tinit: SIGCHLD handler: PID %ld terminated\n", (long) pid);
        }
    }
}

static char** expand_words(char* cmd, wordexp_t* pwordexp)
{
    char** arg_vec = NULL;
    int s = 0;
    
    s = wordexp(cmd, pwordexp, 0);
    if(0 != s)
    {
        fprintf(stderr, "Word expansion failed\n");
        return  NULL;
    }
    
    arg_vec = calloc(pwordexp->we_wordc + 1, sizeof(char*));
    if(NULL == arg_vec)
    {
        errExit("calloc");
    }
    
    for(s = 0; s < pwordexp->we_wordc; s++)
    {
        arg_vec[s] = pwordexp->we_wordv[s];
    }
    
    arg_vec[pwordexp->we_wordc] = NULL;
    
    return arg_vec;
}

static void usage(char* pname)
{
    fprintf(stderr, "Usage: %s [-q]\n", pname);
    fprintf(stderr, "\t-v\tProvide verbose logging\n");
    
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    #define CMD_SIZE (10000)
    struct sigaction sa;
    char cmd[CMD_SIZE] = {'\0'};
    pid_t pid;
    int opt = 0;
    wordexp_t pwordexp;
    
    memset(&pwordexp, 0, sizeof(wordexp_t));
    memset(&sa, 0, sizeof(struct sigaction));
    memset(&pid, 0, sizeof(pid_t));
    
    while(-1 != (opt = getopt(argc, argv, "v")))
    {
        switch(opt)
        {
            case 'v': verbose = 1; break;
            default: usage(argv[0]);
        }
    }
    
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = child_handler;
    if(-1 == sigaction(SIGCHLD, &sa, NULL))
    {
        errExit("sigaction");
    }
    
    if(verbose)
    {
        printf("\tinit: my PID is %ld\n", (long)getpid());
    }
    
    signal(SIGTTOU, SIG_IGN);
    
    if(-1 == setpgid(0, 0))
    {
        errExit("setpgid");
    }
    
    if(-1 == tcsetpgrp(STDIN_FILENO, getpgrp()))
    {
        errExit("tcsetpgrp");
    }
    
    while(1)
    {
        printf("init$");
        if(NULL == fgets(cmd, CMD_SIZE, stdin))
        {
            if(verbose)
            {
                printf("\tinit: exiting");
            }
            printf("\n");
            exit(EXIT_SUCCESS);
        }
        
        if('\n' == cmd[strlen(cmd) - 1])
        {
            cmd[strlen(cmd) - 1] = '\0';
        }
        
        if(0 == strlen(cmd))
        {
            continue;
        }
        
        pid = fork();
        if(-1 == pid)
        {
            errExit("fork");
        }
        
        if(0 == pid)
        {
            char** arg_vec = NULL;
            memset(&pwordexp, 0, sizeof(wordexp_t));

            arg_vec = expand_words(cmd, &pwordexp);
            if(NULL == arg_vec)
            {
                continue;
            }
            
            if(-1 == setpgid(0, 0))
            {
                errExit("setpgid");
            }
            
            if(-1 == tcsetpgrp(STDIN_FILENO, getpgrp()))
            {
                errExit("tcsetpgrp-child");
            }
            
            execvp(arg_vec[0], arg_vec);
            
            if(NULL != arg_vec)
            {
                free(arg_vec);
                arg_vec = NULL;
            }
            wordfree(&pwordexp);
            errExit("execvp");
        }
        
        if(verbose)
        {
            printf("\ninit: created child %ld\n", (long) pid);
        }
        
        pause();
        
        if(-1 == tcsetpgrp(STDIN_FILENO, getpgrp()))
        {
            errExit("tcsetpgrp-parent");
        }
    }
}