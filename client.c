#include "domain_socket.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <fcntl.h>
#include <poll.h>

#define BUFFER_SIZE 256

void client(char *filename)
{
    char msg[MAX_BUF_SZ];
    int amnt = 0, socket_desc;

    socket_desc = domain_socket_client_create(filename);
    if (socket_desc < 0)
        exit(EXIT_FAILURE);

    // successful connection
    printf("User%d connected to server.\n", getpid());
    fflush(stdout);

    // Send our PID to the server so it can set up its client struct
    if (send_pid(socket_desc) < 0)
    {
        close(socket_desc); // Error condition, we want to break the connection
        exit(EXIT_FAILURE);
    }
    fflush(stdout);

    snprintf(msg, MAX_BUF_SZ - 1, "yo what's up");
    amnt = write(socket_desc, msg, strlen(msg) + 1);
    if (amnt < 0)
        exit(EXIT_FAILURE);
    printf("Message Sent!\n");
    fflush(stdout);

    // if (read(socket_desc, msg, amnt) < 0) exit(EXIT_FAILURE);
    // msg[amnt] = '\0';
    //  Receive file descriptor
    int fd = recv_fd(socket_desc);
    printf("Received file descriptor: %d\n", fd);
    printf("User%d reply received from server: %s\n", getpid(), msg);
    fflush(stdout);

    // Set non-blocking mode so we can keep reading even if no new data arrives
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    printf("Listening for updates...\n");

    char buffer[BUFFER_SIZE];
    while (1)
    {
        int ready = poll(&pfd, 1, -1); // Wait indefinitely for data
        if (ready > 0 && (pfd.revents & POLLIN))
        {
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0)
            {
                printf("//START OF DATA//\n");
                buffer[bytes_read] = '\0';
                printf("%s", buffer); // Print new data immediately
                printf("//END OF DATA//\n\n");
                fflush(stdout);
            }
        }
    }

    close(fd);
    close(socket_desc);

    exit(EXIT_SUCCESS);
}

int main(void)
{
    char *channel_name = "db_server";
    int nclients = 1;
    int i;

    /* wait for the server to create the domain socket */
    sleep(1);
    for (i = 0; i < nclients; i++)
    {
        if (fork() == 0)
            client(channel_name);
    }
    /* wait for all of the children */
    while (wait(NULL) != -1);

    return 0;
}