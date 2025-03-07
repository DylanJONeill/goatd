#include "domain_socket.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
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

#define MAX_BUF_SZ 128
#define MAX_FDS 16

typedef enum
{
    READER,
    WRITER
} ClientType;

typedef struct
{
    int domain_socket;
    int pid;
    ClientType type;
} Client;

void unlink_domain_socket(int status, void *filename)
{
    unlink(filename);
    free(filename);
}

void sigint_handler(int sig)
{
    exit(0);
}

// Callback function to print results of the SELECT query
static int callback(void *data, int argc, char **argv, char **azColName)
{
    FILE *output_file = (FILE *)data;

    for (int i = 0; i < argc; i++)
    {
        fprintf(output_file, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    // fprintf(output_file, "\n");
    return 0;
}

void server(char *filename)
{
    char buf[MAX_BUF_SZ];
    char type_buf[MAX_BUF_SZ];
    struct pollfd poll_fds[MAX_FDS];
    int socket_desc, num_fds = 0;
    Client client_list[MAX_CLIENTS];

    for (int j = 0; j < MAX_CLIENTS; ++j)
    {
        client_list[j].domain_socket = -1;
        client_list[j].pid = -1;
    }

    socket_desc = domain_socket_server_create(filename);
    if (socket_desc < 0)
        exit(EXIT_FAILURE);
    on_exit(unlink_domain_socket, strdup(filename));

    memset(poll_fds, 0, sizeof(struct pollfd) * MAX_FDS);
    poll_fds[num_fds++] = (struct pollfd){.fd = socket_desc, .events = POLLIN};

    // Make sure the query result files exist
    FILE *public_file = fopen("query_results.txt", "w");
    if (public_file)
    {
        fprintf(public_file, "# SQL Query Results\n");
        fclose(public_file);
    }

    printf("SQL Server Starting!\n");
    while (1)
    {

        printf("Awaiting connections...\n");
        int ret = poll(poll_fds, num_fds, -1);
        printf("Poll returned: %d\n", ret);

        if (ret == -1)
            exit(EXIT_FAILURE);

        // Accept new connection
        if (poll_fds[0].revents & POLLIN)
        {
            int new_client = accept(socket_desc, NULL, NULL);
            if (new_client == -1)
                exit(EXIT_FAILURE);

            int client_pid = rec_pid(new_client, buf, MAX_BUF_SZ - 1);

            // Read client type (READER or WRITER)
            memset(type_buf, 0, MAX_BUF_SZ);
            if (read(new_client, type_buf, MAX_BUF_SZ - 1) <= 0)
            {
                perror("read client type");
                close(new_client);
                continue;
            }

            ClientType client_type = (strcmp(type_buf, "WRITER") == 0) ? WRITER : READER;
            printf("New client connected: PID %d, Type: %s\n",
                   client_pid, client_type == WRITER ? "WRITER" : "READER");
            fflush(stdout);

            // Handle READER clients immediately
            if (client_type == READER)
            {
                const char *file_to_open = "query_results.txt";

                printf("READER client: sending %s file descriptor\n", file_to_open);
                fflush(stdout);
                int fd = open(file_to_open, O_RDONLY);
                if (fd != -1)
                {
                    send_fd(new_client, fd);
                    close(fd);
                }
                else
                {
                    perror("open file for reader");
                }

                // No need to keep connection with readers
                close(new_client);
                continue;
            }

            // For WRITER clients, store in our client list
            int client_slot = -1;
            for (int j = 0; j < MAX_CLIENTS; ++j)
            {
                if (client_list[j].pid == -1)
                {
                    client_list[j].domain_socket = new_client;
                    client_list[j].pid = client_pid;
                    client_list[j].type = client_type;
                    client_slot = j;
                    break;
                }
            }

            if (client_slot == -1)
            {
                // No slots available
                printf("No client slots available, closing connection\n");
                close(new_client);
                continue;
            }

            // Add writer client to poll set
            poll_fds[num_fds++] = (struct pollfd){.fd = new_client, .events = POLLIN};
            poll_fds[0].revents = 0;
        }

        // Handle data from existing clients (writers)
        for (int i = 1; i < num_fds; i++)
        {
            if (poll_fds[i].revents & POLLIN)
            {
                int client_fd = poll_fds[i].fd;
                char query[MAX_BUF_SZ];

                ssize_t bytes_read = read(client_fd, query, MAX_BUF_SZ - 1);
                if (bytes_read <= 0)
                {
                    // Client disconnected or error
                    if (bytes_read == 0)
                        printf("Client disconnected\n");
                    else
                        perror("read from client");

                    // Remove client from list
                    for (int j = 0; j < MAX_CLIENTS; ++j)
                    {
                        if (client_list[j].domain_socket == client_fd)
                        {
                            client_list[j].domain_socket = -1;
                            client_list[j].pid = -1;
                            break;
                        }
                    }

                    // Close the socket and remove from poll set
                    close(client_fd);
                    poll_fds[i] = poll_fds[num_fds - 1];
                    num_fds--;
                    i--;
                    continue;
                }

                // Process query from writer
                query[bytes_read] = '\0';
                printf("Received query: %s\n", query);

                // Identify which client sent this query
                int client_idx = -1;
                for (int j = 0; j < MAX_CLIENTS; ++j)
                {
                    if (client_list[j].domain_socket == client_fd)
                    {
                        client_idx = j;
                        break;
                    }
                }

                if (client_idx == -1)
                {
                    fprintf(stderr, "Could not find client data for fd %d\n", client_fd);
                    continue;
                }

                // Write query to appropriate file based on client PID
                const char *file_to_update = "query_results.txt";

                FILE *fp = fopen(file_to_update, "a");
                if (fp)
                {
                    fprintf(fp, "Query from client %d: %s\n", client_list[client_idx].pid, query);

                    // The file is open, let's make the query now
                    // Open the database to prepare to write to it
                    sqlite3 *db;
                    char *zErrMsg = 0;
                    int rc = sqlite3_open("test.db", &db);
                    if (rc)
                    {
                        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
                    }
                    else
                    {
                        fprintf(stderr, "Opened database successfully\n");
                    }
                    rc = sqlite3_exec(db, query, callback, (void *)fp, &zErrMsg);
                    if (rc != SQLITE_OK)
                    {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                    }
                    else
                    {
                        fprintf(stderr, "Query executed successfully, results written to file\n");
                    }

                    sqlite3_close(db);

                    fclose(fp);
                    printf("Query appended to %s\n", file_to_update);
                }
                else
                {
                    perror("fopen for query");
                }
            }
        }
    }
    close(socket_desc);
}

int main(void)
{
    char *channel_name = "db_server";

    if (signal(SIGINT, sigint_handler) == SIG_ERR)
    {
        perror("signal");
        return 1;
    }

    if (fork() == 0)
    {
        server(channel_name);
    }

    while (wait(NULL) != -1);
    return 0;
}