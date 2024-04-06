#include <stdio.h>
#include <stdlib.h>

#include "../src/guava.h"

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