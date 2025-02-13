#include "domain_socket.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>



void
client(char *filename)
{
    char msg[MAX_BUF_SZ];
    int  amnt = 0, socket_desc;

    socket_desc = domain_socket_client_create(filename);
    if (socket_desc < 0) exit(EXIT_FAILURE);

    //successful connection
    printf("User%d connected to server.\n", getpid());
    fflush(stdout);

    snprintf(msg, MAX_BUF_SZ - 1, "yo what's up");
    amnt = write(socket_desc, msg, strlen(msg) + 1);
    if (amnt < 0) exit(EXIT_FAILURE);
    printf("Message Sent!\n");
    fflush(stdout);

    if (read(socket_desc, msg, amnt) < 0) exit(EXIT_FAILURE);
    msg[amnt] = '\0';
    printf("User%d reply received from server: %s\n", getpid(), msg);
    fflush(stdout);

    close(socket_desc);

    exit(EXIT_SUCCESS);
}

int
main(void)
{
    char *channel_name = "db_server";
    int nclients = 2;
    int i;

    /* wait for the server to create the domain socket */
    sleep(1);
    for (i = 0; i < nclients; i++) {
        if (fork() == 0) client(channel_name);
    }
    /* wait for all of the children */
    while (wait(NULL) != -1);

    return 0;
}