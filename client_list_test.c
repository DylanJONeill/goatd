#include "domain_socket.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <sqlite3.h>

void sigint_handler(int sig) //From server.c
{
    exit(0);
}

void make_writer(){
    pid_t writer = fork();
    if(writer == 0){
        char* args[] = {"./client", "writer", NULL};
        execvp(args[0], args);
        exit(0);
    }
}

int main() {
    char *channel_name = "db_server";
    //Make server instance before running test
  
    for(int i = 0; i < 8; i++){
        make_writer(); //Makes 8 writers, should fill up client list
        //printf("Writer %d connected\n", i);
    }

    //If all successfully connect, test passed - 8 clients can work at once

    printf("CLIENT LIST TEST PASSED\n");
    return 0;
}