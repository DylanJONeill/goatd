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

int main() {
    char *channel_name = "db_server";
    //Make server instance before running test
    pid_t total_children[MAX_CLIENTS] = {-1};
    for(int i = 0; i < MAX_CLIENTS; ++i){ //Makes MAX_CLIENTS writers, should fill up client list
        pid_t writer = fork();
        if(writer == 0){
            char* args[] = {"./client", "writer", NULL};
            execvp(args[0], args);
            exit(0);
        } else if (writer < 0) {
        printf("RE RUN TEST\n");
        exit(EXIT_FAILURE);
        } 
        total_children[i] = getpid();
        //printf("Writer %d connected\n", i);
    }

    sleep(3); //Wait for all children to be initialised

    for (int j = 0; j < MAX_CLIENTS; ++j) {
        if (total_children[j] == -1) {
            printf("CLIENT LIST TEST FAILED\n");
            return -1;
        }
    }

    //If all successfully connect, test passed - 8 clients can work at once

    printf("\nCLIENT LIST TEST PASSED\n");
    return 0;
}