# Guava

Guava is a simple to use event loop library written in C. It aims to provide a thin abstraction over platform APIs.

## Status

Guava is currently under active development and should not be used in production.

## Features

Currently Guava supports file descriptor events and time events, file system events are currently not supported but I am planning to implement a thread pool to support those as well, though it is not an immediate priority. Currently I'm focussing on adding more backends and stabilizing the system.

## Examples

Fully working code can be found in the [examples](./examples/) directory.

### Timers

```c

int interval_callback(GuavaLoop *loop, int id, void *data) {
    printf("interval callback called for time event %d\n", id);

    return 100;
}

int timeout_callback(GuavaLoop *loop, int id, void *data) {
    printf("timeout callback called for time event %d\n", id);

    return GUAVA_STOP_TIMER;
}

int main() {
    GuavaLoop *loop = guava_create_loop(1024);
    guava_create_time_event(loop, 1000, NULL, interval_callback);
    guava_create_time_event(loop, 10000, NULL, timeout_callback);
    guava_start_loop(loop);
    guava_delete_loop(loop);
    return 0;
}
```

```
interval callback called for time event 0
interval callback called for time event 0
interval callback called for time event 0
interval callback called for time event 0
interval callback called for time event 0
interval callback called for time event 0
interval callback called for time event 0
interval callback called for time event 0
interval callback called for time event 0
timeout callback called for time event 1
```

### TCP Server

```c
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
```

```
new connection
new connection
```

## Supported Backend

1. Kqueue - On macOS and BSD systems
2. Poll - On rest of the unix platforms
3. Epoll - Under development

The best backend for your system is automatically identified and used by Guava.
