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

