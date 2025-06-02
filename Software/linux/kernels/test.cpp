#ifndef _GNU_SOURCE
	#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif

#include <stack>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
#include "libraries/ght.h"
#include "libraries/ghe.h"
#include "libraries/gc_top.h"
#include "libraries/deque.hpp"

int main(void) {
    int cap = 700;
    int num_queues = 3;

    // Bound current thread to BOOM
    if (gc_pthread_setaffinity(BOOM_ID) != 0) {
        printf("[BOOM-C%x]: pthread_setaffinity failed.", BOOM_ID);
    }

    Deque shadow;
    Deque queues[num_queues];
    Deque shadow_agg;
    uint64_t sum[3] = {0, 0, 0}; // initialize sum array

    for (uint64_t i = 0; i < num_queues; i++) {
        for (uint64_t j = 100 * i; j < cap; j++) {
            push_front(&queues[i], j);
        }
    }

    for (int i = 0; i < num_queues; i++) {
        while (!is_empty(&queues[i])) {
            uint64_t data;
            pop_back(&queues[i], &data);
            push_front(&shadow, data);
            push_back(&shadow_agg, data);
        }
    }

    while (!is_empty(&shadow)) {
        uint64_t data;
        pop_back(&shadow, &data);
        sum[0] += data; // shorthand for sum[0] = sum[0] + data;
        sum[2] += data;
    }

    while (!is_empty(&shadow_agg)) {
        uint64_t data;
        pop_front(&shadow_agg, &data);
        sum[1] += data; // shorthand for sum[1] = sum[1] + data;
        sum[2] += data;
    }

    printf("Results: %lu, %lu, %lu", sum[0], sum[1], sum[2]); // use %lu for uint64_t

    return 0;
}