#include <stdlib.h>
#include <sys/event.h>
#include <unistd.h>

#include "guava.h"

typedef struct GuavaBackend {
    int kqueue_fd;
    struct kevent* events;
} GuavaBackend;

static int guava_initialize_backend(GuavaLoop* loop) {
    int kqueue_fd = kqueue();

    if (kqueue_fd == -1) {
        return -1;
    }

    GuavaBackend* backend = malloc(sizeof(GuavaBackend));
    if (backend == NULL) {
        close(kqueue_fd);
        return -1;
    }

    backend->events = malloc(sizeof(struct kevent) * loop->fd_events_set_size);

    if (backend->events == NULL) {
        return -1;
    }

    backend->kqueue_fd = kqueue_fd;
    loop->backend = backend;
    return 0;
}

static int create_backend_fd_event(GuavaLoop* loop, int fd, int mask) {
    GuavaBackend* backend = loop->backend;
    struct kevent event;

    if (mask & GUAVA_READABLE) {
        EV_SET(&event, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent(backend->kqueue_fd, &event, 1, NULL, 0, NULL) == -1) {
            return -1;
        }
    }

    if (mask & GUAVA_WRITABLE) {
        EV_SET(&event, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
        if (kevent(backend->kqueue_fd, &event, 1, NULL, 0, NULL) == -1) {
            return -1;
        }
    }

    return 0;
}

static int backend_poll(GuavaLoop* loop) {
    GuavaBackend* backend = loop->backend;
    struct timespec t;
    t.tv_sec = 1;
    int n = kevent(backend->kqueue_fd, NULL, 0, backend->events, loop->fd_events_set_size, &t);

    for (int i = 0; i < n; i++) {
        struct kevent* event = backend->events + i;
        loop->fired_fd_events[i].fd = event->ident;
        if (event->filter & EVFILT_READ) {
            loop->fired_fd_events[i].mask = GUAVA_READABLE;
        }

        if (event->filter & EVFILT_WRITE) {
            loop->fired_fd_events[i].mask = GUAVA_WRITABLE;
        }
    }

    return n;
}

static void delete_backend(GuavaLoop* loop) {
    GuavaBackend* backend = loop->backend;
    close(backend->kqueue_fd);
    free(backend->events);
    free(backend);
}