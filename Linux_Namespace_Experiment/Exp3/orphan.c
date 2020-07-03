/* orphan.c

   Copyright 2013, Michael Kerrisk
   Licensed under GNU General Public License v2 or later

   Demonstrate that a child becomes orphaned (and is adopted by init(1),
   whose PID is 1) when its parent exits.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    pid_t pid;
    
    memset(&pid, 0, sizeof(pid_t));
    
    pid = fork();
    if(-1 == pid)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if(0 != pid)
    {
        printf("Parent (PID = %ld) created child with PID %ld\n", 
                (long)getpid(), (long)pid);
        printf("Parent (PID = %ld; PPID = %ld) terminating\n",
                (long)getpid(), (long)getppid());
        exit(EXIT_SUCCESS);
    }
    
    do{
        usleep(10000);
    }while(1 != getppid());
        
    printf("\nChild (PID=%ld) now an orphan (parent PID=%ld)\n",
            (long) getpid(), (long)getppid());
    
    sleep(1);
    
    printf("Child (PID=%ld) terminating\n", (long)getpid());
    _exit(EXIT_SUCCESS);
}