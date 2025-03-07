#pragma once

/*
 * Graciously taken from
 * https://github.com/troydhanson/network/tree/master/unixdomain
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#define MAX_BUF_SZ 128

#define PID_REQUEST "WRITE PID TO DESCRIPTOR"
#define PID_ACKNOWLEDGE "SERVER RECEIVED PID"
#define MAX_CLIENTS 8

struct client {
    int pid;
    int domain_socket;
};

void server(char *filename);

int
domain_socket_client_create(const char *file_name)
{
	struct sockaddr_un addr;
	int fd;

	/* Create the socket descriptor.  */
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) return -1;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, file_name, sizeof(addr.sun_path) - 1);

	/* Attempt to connect the socket descriptor with a socket file named `file_name`. */
	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) return -1;

	return fd;
}

int
domain_socket_server_create(const char *file_name)
{
	struct sockaddr_un addr;
	int fd;

	/* Create the socket descriptor.  */
	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) return -1;

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, file_name, sizeof(addr.sun_path) - 1);

	/* Associate the socket descriptor with a socket file named `file_name`. */
	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) return -1;
	/* How many clients can the system queue up before they are `accept`ed? */
	if (listen(fd, 5) == -1) return -1;

	return fd;
}

// Used by the server to send the file descriptor of the opened file to the respective client
void send_fd(int socket, int fd){
	struct msghdr msg = {0};
    struct iovec io;
    char buf[1] = {0};
	char cmsg_buf[CMSG_SPACE(sizeof(int))];

	msg.msg_control = cmsg_buf;
	msg.msg_controllen = sizeof(cmsg_buf);

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    // Copy file descriptor into message
    *((int *) CMSG_DATA(cmsg)) = fd;

	// Add dummy data to satisfy `sendmsg()`
    io.iov_base = buf;
    io.iov_len = sizeof(buf);
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    if (sendmsg(socket, &msg, 0) == -1) {
        perror("sendmsg");
        exit(EXIT_FAILURE);
    }
}

int recv_fd(int socket){
	struct msghdr msg = {0};
    struct iovec io;
    char buf[1]; // Dummy byte
    char cmsg_buf[CMSG_SPACE(sizeof(int))];

    msg.msg_control = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    io.iov_base = buf;
    io.iov_len = sizeof(buf);
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    if (recvmsg(socket, &msg, 0) == -1) {
        perror("recvmsg");
        exit(EXIT_FAILURE);
    }

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg == NULL || cmsg->cmsg_len != CMSG_LEN(sizeof(int))) {
        fprintf(stderr, "No file descriptor received\n");
        exit(EXIT_FAILURE);
    }

    int fd = *((int *) CMSG_DATA(cmsg));
    return fd;
}

int server_request_pid(int domain_socket) {
    char* pid_write_request = PID_REQUEST;
    return write(domain_socket, pid_write_request, strlen(pid_write_request));
}

int send_pid(int domain_socket) {
    pid_t client_pid = getpid();
    char pid_buf[64];
    sprintf(pid_buf, "%d", client_pid);
    //printf("pid_buf: %s\n", pid_buf);
    char pid_request_buf[strlen(PID_REQUEST)];
    char pid_acknowledge_buf[strlen(PID_ACKNOWLEDGE)];
    //printf("client_pid: %ls\n", &client_pid);
    for (;;) {
        read(domain_socket, pid_request_buf, strlen(PID_REQUEST));
        if (strcmp(pid_request_buf, PID_REQUEST) == 0) { //If we get the PID request, write out pid
            write(domain_socket, pid_buf, strlen(pid_buf));
            break;
        }
    } //Sent the pid, wait for server to acknowledge it
    //printf("PID sent\n");
    for (;;) {
        read(domain_socket, pid_acknowledge_buf, strlen(PID_ACKNOWLEDGE));
        if (strcmp(pid_acknowledge_buf, PID_ACKNOWLEDGE) == 0) { //If we get the PID acknowledge, continue
            break;
        }
    }
    //printf("PID acknowledged\n");

    return 0;
}

int rec_pid(int domain_socket, char* buf, size_t buf_size) {
    //printf("domain socket in rec_pid: %d\n", domain_socket);
    if (server_request_pid(domain_socket) <=0) {
        close(domain_socket); //Error condition, we want to break connection
        exit(EXIT_FAILURE);
    }
    
    fflush(stdout);

    if (read(domain_socket, buf, buf_size) <= 0) {
        close(domain_socket); //Error condition, we want to break connection
        exit(EXIT_FAILURE);
    }

    long pid = strtol(buf, &buf, 10);
    //printf("received pid: %ld\n", pid);
    //Let client know we've received their PID
    if (write(domain_socket, PID_ACKNOWLEDGE, strlen(PID_ACKNOWLEDGE)) <= 0) {
        close(domain_socket); //Error condition, we want to break connection
        exit(EXIT_FAILURE);
    }

    return (int)pid;
}