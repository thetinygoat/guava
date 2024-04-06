#include "guava.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(__APPLE__)
#include "guava_kqueue.c"
#else
#include "guava_poll.c"
#endif

GuavaLoop* guava_create_loop(int set_size) {
    GuavaLoop* loop = malloc(sizeof(*loop));
    if (loop == NULL) {
        return NULL;
    }

    loop->fd_events = malloc(sizeof(GuavaFdEvent) * set_size);
    loop->fired_fd_events = malloc(sizeof(GuavaFiredEvent) * set_size);

    if (loop->fd_events == NULL || loop->fired_fd_events == NULL) {
        return NULL;
    }

    //   prefill the fd events with GUAVA_NOOP mask, so that they won't be
    //   processed

    for (int i = 0; i < set_size; i++) {
        loop->fd_events[i].mask = GUAVA_NOOP;
    }

    loop->fd_events_set_size = set_size;
    loop->running = 0;

    if (guava_initialize_backend(loop) == -1) {
        free(loop->fd_events);
        free(loop->fired_fd_events);
        free(loop);
        return NULL;
    }

    return loop;
}

void guava_delete_loop(GuavaLoop* loop) {
    loop->running = 0;
    delete_backend(loop);
    free(loop->fd_events);
    free(loop->fired_fd_events);
    free(loop);
}

int guava_create_fd_event(GuavaLoop* loop, int fd, int mask, void* data, void* callback) {
    int retval = create_backend_fd_event(loop, fd, mask);
    if (retval < 0) {
        return retval;
    }
    loop->fd_events[fd].fd = fd;
    loop->fd_events[fd].data = data;
    loop->fd_events[fd].mask = mask;
    loop->fd_events[fd].callback = callback;

    return 0;
}

static long get_monotonic_millis() {
    struct timespec t;
    int res = clock_gettime(CLOCK_MONOTONIC_RAW, &t);

    if (res == -1) {
        return -1;
    }

    return t.tv_sec * 1000;
}

int guava_create_time_event(GuavaLoop* loop, long millis, void* data, time_callback* callback) {
    GuavaTimeEvent* event = malloc(sizeof(GuavaTimeEvent));
    if (event == NULL) {
        return -1;
    }
    long now = get_monotonic_millis();
    if (now == -1) {
        return -1;
    }
    event->id = loop->next_time_event_id++;
    event->callback = callback;
    event->when = now + millis;
    event->data = data;
    if (loop->time_events == NULL) {
        event->next = NULL;
        event->prev = NULL;
        loop->time_events = event;
    } else {
        event->prev = NULL;
        event->next = loop->time_events;
        loop->time_events->prev = event;
        loop->time_events = event;
    }
    return event->id;
}

static void process_time_events(GuavaLoop* loop) {
    if (loop->time_events == NULL) return;

    long now = get_monotonic_millis();
    GuavaTimeEvent* te = loop->time_events;
    long max_id = loop->next_time_event_id - 1;
    while (te != NULL) {
        // delete the event if it is scheduled to be deleted
        if (te->id == GUAVA_DELETED_EVENT_ID) {
            GuavaTimeEvent* next = te->next;
            if (te->prev) {
                te->prev->next = te->next;
            } else {
                loop->time_events = te->next;
            }

            if (te->next) {
                te->next->prev = te->prev;
            }
            free(te);
            te = next;
            continue;
        }
        // don't process the events added in this tick
        if (te->id > max_id) {
            continue;
        }
        if (now >= te->when) {
            int ret = te->callback(loop, te->id, te->data);
            long now = get_monotonic_millis();
            if (ret != GUAVA_STOP_TIMER) {
                te->when = now + ret;
            } else {
                te->id = GUAVA_DELETED_EVENT_ID;
            }
        }
        te = te->next;
    }
}

static int process_events(GuavaLoop* loop) {
    int n = backend_poll(loop);
    if (n < 0) return n;

    for (int i = 0; i < n; i++) {
        GuavaFiredEvent* fe = &loop->fired_fd_events[i];
        GuavaFdEvent* event = &loop->fd_events[fe->fd];

        if ((event->mask & GUAVA_READABLE) || (event->mask & GUAVA_WRITABLE)) {
            event->callback(loop, event->fd, event->mask, event->data);
        }
    }

    process_time_events(loop);

    return n;
}

void guava_start_loop(GuavaLoop* loop) {
    loop->running = 1;
    while (loop->running) {
        process_events(loop);
    }
}
