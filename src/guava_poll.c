#include <poll.h>
#include <stdio.h>
#include <stdlib.h>

#include "guava.h"

typedef struct GuavaBackend {
    struct pollfd* fds;
} GuavaBackend;

static int guava_initialize_backend(GuavaLoop* loop) {
    GuavaBackend* backend = malloc(sizeof(GuavaBackend));
    if (backend == NULL) {
        return -1;
    }

    backend->fds = malloc(sizeof(struct pollfd) * loop->fd_events_set_size);

    if (backend->fds == NULL) {
        free(backend);
        return -1;
    }

    for (int i = 0; i < loop->fd_events_set_size; i++) {
        // set the fd to -1 so that it will be ignored;
        // set the events bitmask to 0
        struct pollfd* event = backend->fds + i;
        event->fd = -1;
        event->events = 0;
    }

    loop->backend = backend;

    return 0;
}

static int create_backend_fd_event(GuavaLoop* loop, int fd, int mask) {
    GuavaBackend* backend = loop->backend;

    struct pollfd* event = backend->fds + fd;

    event->fd = fd;

    if (mask & GUAVA_READABLE) {
        event->events |= POLLIN;
    }

    if (mask & GUAVA_WRITABLE) {
        event->events |= POLLOUT;
    }

    return 0;
}

static int backend_poll(GuavaLoop* loop) {
    GuavaBackend* backend = loop->backend;

    int n = poll(backend->fds, loop->fd_events_set_size, 1000);

    int fired = 0;
    for (int i = 0; i < loop->fd_events_set_size; i++) {
        struct pollfd* event = backend->fds + i;
        if (event->fd == -1) {
            continue;
        }
        loop->fired_fd_events[fired].fd = event->fd;

        if (event->revents & POLLIN) {
            loop->fired_fd_events[i].mask = GUAVA_READABLE;
        }

        if (event->revents & POLLOUT) {
            loop->fired_fd_events[i].mask = GUAVA_WRITABLE;
        }
        fired++;
    }

    return n;
}

static void delete_backend(GuavaLoop* loop) {
    GuavaBackend* backend = loop->backend;
    free(backend->fds);
    free(backend);
}