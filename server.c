#include "domain_socket.h"

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <poll.h>

#define MAX_BUF_SZ 128
#define MAX_CLIENTS 8
#define MAX_FDS 16

void
unlink_domain_socket(int status, void *filename)
{
    unlink(filename);
    free(filename);
}

void
server(int num_clients, char *filename)
{
    char buf[MAX_BUF_SZ];
    struct pollfd poll_fds[MAX_FDS];
    int new_client, amnt, i, socket_desc;
    struct client client_list[MAX_CLIENTS];

    for (int j = 0; j < MAX_CLIENTS; ++j) {
        client_list[j].domain_socket = -5;
        client_list[j].pid = -5;
    }

    socket_desc = domain_socket_server_create(filename);
    int num_fds = 0;

    /* should do proper cleanup */
    if (socket_desc < 0){
        exit(EXIT_FAILURE); 
    } 
    on_exit(unlink_domain_socket, strdup(filename));

    //init the poll fds
    memset(poll_fds, 0, sizeof(struct pollfd) * MAX_FDS);

    //the service is itself is a file descriptor, so add to the table of fds
    poll_fds[num_fds++] = (struct pollfd) {
        .fd     = socket_desc,
        .events = POLLIN,
    };

    printf("SQL Server Starting!\n");
    for (;;) {

        printf("Polling...\n");
        int ret = poll(poll_fds, num_fds, -1);
        if (ret == -1) exit(EXIT_FAILURE);
        //printf("ret: %d\n", ret);
        printf("Polled something\n");
        
        //accept a new client
        if (poll_fds[0].revents & POLLIN) {

            if ((new_client = accept(socket_desc, NULL, NULL)) == -1){
                exit(EXIT_FAILURE);
            }

            //Set up our client struct in an available index
            for (int j = 0; j < MAX_CLIENTS; ++j) {
                if (client_list[j].pid == -5) {
                    //printf("j = %d\n", j);
                    client_list[j].domain_socket = new_client; //Sets our FD to the socket FD
                    fflush(stdout);
                    client_list[j].pid = rec_pid(new_client, buf, MAX_BUF_SZ - 1); //Get the PID from the client
                    break; //Move on, we've already got a spot for the new client
                } else if (j == MAX_CLIENTS - 1) { //No client spots available, kill the client connection
                    close(new_client);
                }
            }
            //Client set up complete, now we can do our work
            //printf("Client setup complete\n");
            fflush(stdout);

            //add it to the polling list
            poll_fds[num_fds++] = (struct pollfd) {
                .fd = new_client,
                .events = POLLIN
            };
            poll_fds[0].revents = 0;

            //successful connection
            printf("Server: New client connected with new file descriptor %d.\n", new_client);
            fflush(stdout);
        }

        //read the message
        /*
        amnt = read(new_client, buf, MAX_BUF_SZ - 1);
        if (amnt == -1) exit(EXIT_FAILURE);
        buf[amnt] = '\0'; 
        printf("Server received message (sz %d): \"%s\". Replying!\n", amnt, buf);
        fflush(stdout);
        */

        // Open file to send
        int fd = open("query_results.txt", O_RDONLY);
        if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        /* send the client a reply */
        // if (write(new_client, buf, amnt) < 0) exit(EXIT_FAILURE);
        send_fd(new_client, fd);
        printf("Received file descriptor: %d\n", fd);
        /* Done with communication with this client */
        //Now we need to remove the client from our active client list
        for (int j = 0; j < MAX_CLIENTS; ++j) {
            if (client_list[j].domain_socket == new_client) {
                client_list[j].pid = -5; //Set slot open for new clients
                client_list[j].domain_socket = -5;
            }
        }
        //Client removed from active client list
        close(new_client);
    }
    close(socket_desc);

    exit(EXIT_SUCCESS);
}



int
main(void)
{
    char *channel_name = "db_server";
    int nclients = 2;
    int i;

    //start the
    if (fork() == 0){
        server(nclients, channel_name);
    }
    /* wait for the server to create the domain socket */


    /* wait for all of the children */
    while (wait(NULL) != -1);

    return 0;
}