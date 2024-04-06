#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "../src/guava.h"

void callback(GuavaLoop *loop, int fd, int mask, void *data) {
    struct sockaddr_storage incoming;
    socklen_t size = sizeof(incoming);
    printf("new connection\n");
    int conn_fd = accept(fd, (struct sockaddr *)&incoming, &size);
    if (conn_fd == -1) {
        return;
    }
}

int main() {
    struct addrinfo hints;
    struct addrinfo *serv;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, "8080", &hints, &serv);

    if (status != 0) {
        exit(1);
    }

    int sock_fd = socket(serv->ai_family, serv->ai_socktype, serv->ai_protocol);

    if (sock_fd == -1) {
        exit(1);
    }

    int res = bind(sock_fd, serv->ai_addr, serv->ai_addrlen);

    if (res == -1) {
        exit(1);
    }

    int res1 = listen(sock_fd, 5);

    if (res1 == -1) {
        exit(1);
    }

    GuavaLoop *loop = guava_create_loop(1024);
    guava_create_fd_event(loop, sock_fd, GUAVA_READABLE, NULL, callback);
    guava_start_loop(loop);
    guava_delete_loop(loop);
    close(sock_fd);

    return 0;
}