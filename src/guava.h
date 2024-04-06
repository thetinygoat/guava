#include <stdint.h>

#ifndef GUAVA_H
#define GUAVA_H

#define GUAVA_NOOP 0
#define GUAVA_READABLE 1
#define GUAVA_WRITABLE 2

#define GUAVA_STOP_TIMER -1

#define GUAVA_DELETED_EVENT_ID -1

struct GuavaLoop;

typedef void fd_callback(struct GuavaLoop* loop, int fd, int mask, void* data);
typedef int time_callback(struct GuavaLoop* loop, int id, void* data);

typedef struct GuavaFdEvent {
    int fd;
    int mask;
    fd_callback* callback;
    void* data;
} GuavaFdEvent;

typedef struct GuavaTimeEvent {
    int id;
    time_callback* callback;
    long when;
    void* data;
    struct GuavaTimeEvent* next;
    struct GuavaTimeEvent* prev;
} GuavaTimeEvent;

typedef struct GuavaFiredEvent {
    int fd;
    int mask;
} GuavaFiredEvent;

typedef struct GuavaLoop {
    int fd_events_set_size;
    GuavaFdEvent* fd_events;
    GuavaFiredEvent* fired_fd_events;
    GuavaTimeEvent* time_events;
    int next_time_event_id;
    short int running;
    void* backend;
} GuavaLoop;

GuavaLoop* guava_create_loop(int set_size);
void guava_delete_loop(GuavaLoop* loop);
int guava_create_fd_event(GuavaLoop* loop, int fd, int mask, void* data, void* callback);
void guava_start_loop(GuavaLoop* loop);
int guava_create_time_event(GuavaLoop* loop, long mills, void* data, time_callback* callback);
#endif