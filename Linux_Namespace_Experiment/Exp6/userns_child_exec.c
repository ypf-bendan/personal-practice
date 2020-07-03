/* userns_child_exec.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Create a child process that executes a shell command in new
   namespace(s); allow UID and GID mappings to be specified when
   creating a user namespace.
*/
#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

/* A simple error-handling function: print an error message based
   on the value in 'errno' and terminate the calling process */

#define errExit(msg)                    \
    do                                  \
    {                                   \
        perror(msg);                    \
        exit(EXIT_FAILURE);             \
    } while (0)

struct child_args
{
    char **argv;
    int pipe_fd[2];
};

static int verbose = 0;

static void usage(char *pname)
{
    fprintf(stderr, "Usage: %s [options] cmd [arg...]\n\n", pname);
    fprintf(stderr, "Create a child process that executes a shell command"
            "in a new user namespace,\n"
            "and possibly also other new namespace(s).\n\n");
    fprintf(stderr, "Options can be:\n\n");
#define fpe(str) fprintf(stderr, "\t%s", str)
    fpe("-i         New IPC namespace\n");
    fpe("-m         New mount namespace\n");
    fpe("-n         New network namespace\n");
    fpe("-p         New PID namespace\n");
    fpe("-u         New UTS namespace\n");
    fpe("-U         New user namespace\n");
    fpe("-M         uid_map Specify UID map for user namespace\n");
    fpe("-G         gid_map Specify GID map for user namespace\n");
    fpe("           if -M or -G is specify, -U is required\n");
    fpe("-v         Display verbose message\n");
    fpe("Map strings for -M and -G consist of records of the form:\n");
    fpe("\n");
    fpe("    ID-inside-ns  ID-outside-ns  len\n");
    fpe("\n");
    fpe("A map string can contain multiple records, separated by commas;\n");
    fpe("the commas are replaced by newlines before writing to map files.\n");

    exit(EXIT_FAILURE);
}

static void update_map(char *mapping, char *map_file)
{
    int fd = 0, j = 0;
    size_t map_len = 0;

    map_len = strlen(mapping);
    for(j = 0; j < map_len; j++)
    {
        if(',' == mapping[j])
        {
            mapping[j] = '\n';
        }
    }

    printf("mapping: %s\n", mapping);
    fd = open(map_file, O_RDWR);
    if(-1 == fd)
    {
        fprintf(stderr, "open %s: %s\n", map_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(map_len != write(fd, mapping, map_len))
    {
        fprintf(stderr, "write %s: %s\n", map_file, strerror(errno));
        exit(EXIT_FAILURE);
    }

    close(fd);
    return;
}

static int childFunc(void *arg)
{
    struct child_args * args = (struct child_args*)arg;
    char ch = '\0';

    close(args->pipe_fd[1]);

    if(0 != read(args->pipe_fd[0], &ch, 1))
    {
        fprintf(stderr, "Failure in child: read from pipe returned\n");
        exit(EXIT_FAILURE);
    }

    execvp(args->argv[0], args->argv);
    errExit("execvp");
}

#define STACK_SIZE  (1024 * 1024)
static char child_stack[STACK_SIZE];

int main(int argc, char* argv[])
{
    extern char *optarg;
    extern int optind, opterr, optopt;

    int flags = 0, opt = 0;
    pid_t child_pid;
    struct child_args args;
    char *uid_map = NULL, *gid_map = NULL;
    char map_path[PATH_MAX] = {'\0'};

    memset(&child_pid, 0, sizeof(pid_t));
    memset(&args, 0, sizeof(struct child_args));

    while(-1 != (opt = getopt(argc, argv, "+imnpuUM:G:v")))
    {
        switch(opt)
        {
            case 'i': flags |= CLONE_NEWIPC;        break;
            case 'm': flags |= CLONE_NEWNS;         break;
            case 'n': flags |= CLONE_NEWNET;        break;
            case 'p': flags |= CLONE_NEWPID;        break;
            case 'u': flags |= CLONE_NEWUTS;        break;
            case 'v': verbose = 1;                  break;
            case 'M': uid_map = optarg;             break;
            case 'G': gid_map = optarg;             break;
            case 'U': flags |= CLONE_NEWUSER;       break;
            default:  usage(argv[0]);
        }
    }

    if(( (NULL != uid_map) || (NULL != gid_map)) && 
         !(flags & CLONE_NEWUSER))
    {
        usage(argv[0]);
    }

    args.argv = &argv[optind];
    if(-1 == pipe(args.pipe_fd))
    {
        errExit("pipe failed");
    }

    child_pid = clone(childFunc, child_stack + STACK_SIZE, 
                        flags | SIGCHLD, &args);
    if(-1 == child_pid)
    {
        errExit("clone failed");
    }

    if(verbose)
    {
        printf("%s: PID of child created by clone() is %ld\n", argv[0], (long)child_pid);
    }

    if(NULL != uid_map)
    {
        snprintf(map_path, PATH_MAX, "/proc/%ld/uid_map", (long)child_pid);
        update_map(uid_map, map_path);
    }

    if(NULL != gid_map)
    {
        snprintf(map_path, PATH_MAX, "/proc/%ld/gid_map", (long)child_pid);
        update_map(gid_map, map_path);
    }

    close(args.pipe_fd[1]);

    if(-1 == waitpid(child_pid, NULL, 0))
    {
        errExit("waitpid failed");
    }

    if(verbose)
    {
        printf("%s: terminating\n", argv[0]);
    }

    exit(EXIT_SUCCESS);
}

