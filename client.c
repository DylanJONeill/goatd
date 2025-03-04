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

typedef enum { READER, WRITER } ClientType;

void writer_client(char *filename)
{
    char query[BUFFER_SIZE];
    int socket_desc;

    socket_desc = domain_socket_client_create(filename);
    if (socket_desc < 0)
        exit(EXIT_FAILURE);
    
    printf("Writer client (PID %d) connected.\n", getpid());
    fflush(stdout);

    // Send our PID to the server
    if (send_pid(socket_desc) < 0)
    {
        close(socket_desc);
        exit(EXIT_FAILURE);
    }
    
    // Send client type to server
    const char *type_str = "WRITER";
    if (write(socket_desc, type_str, strlen(type_str) + 1) < 0)
    {
        perror("write client type");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    printf("SQL Writer ready. Enter queries below:\n");
    while (1)
    {
        printf("SQL> ");
        fflush(stdout);
        if (fgets(query, BUFFER_SIZE, stdin) == NULL)
            break; // Exit if input error
        
        if (write(socket_desc, query, strlen(query) + 1) < 0)
        {
            perror("write query");
            break;
        }
    }
    
    close(socket_desc);
    exit(EXIT_SUCCESS);
}

void reader_client(char *filename)
{
    char buffer[BUFFER_SIZE];
    int socket_desc = domain_socket_client_create(filename);
    if (socket_desc < 0)
        exit(EXIT_FAILURE);

    printf("Reader client (PID %d) connected.\n", getpid());
    fflush(stdout);

    // Send our PID to the server
    if (send_pid(socket_desc) < 0)
    {
        close(socket_desc);
        exit(EXIT_FAILURE);
    }
    
    // Send client type to server
    const char *type_str = "READER";
    if (write(socket_desc, type_str, strlen(type_str) + 1) < 0)
    {
        perror("write client type");
        close(socket_desc);
        exit(EXIT_FAILURE);
    }

    // Receive file descriptor
    int fd = recv_fd(socket_desc);
    printf("Received file descriptor: %d\n", fd);
    fflush(stdout);

    // Set non-blocking mode
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    printf("Listening for updates...\n");

    while (1)
    {
        int ready = poll(&pfd, 1, -1); // Wait indefinitely for data
        if (ready > 0 && (pfd.revents & POLLIN))
        {
            ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0)
            {
                printf("\n----START OF DATA----\n");
                buffer[bytes_read] = '\0';
                printf("%s\n", buffer); // Print new data immediately
                printf("\n----END OF DATA----\n");
                fflush(stdout);
            }
        }
    }

    close(fd);
    close(socket_desc);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <reader|writer>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *channel_name = "db_server";
    ClientType type;

    if (strcmp(argv[1], "writer") == 0)
        type = WRITER;
    else if (strcmp(argv[1], "reader") == 0)
        type = READER;
    else {
        fprintf(stderr, "Invalid client type. Use 'reader' or 'writer'\n");
        return EXIT_FAILURE;
    }

    /* wait for the server to create the domain socket */
    sleep(1);
    
    if (fork() == 0)
    {
        if (type == WRITER)
            writer_client(channel_name);
        else
            reader_client(channel_name);
    }
    
    /* wait for all of the children */
    while (wait(NULL) != -1);

    return 0;
}