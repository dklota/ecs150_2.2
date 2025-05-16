#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../libuthread/uthread.h"
#include "../libuthread/sem.h"

#define NUM_ITERATIONS 5
#define NUM_THREADS 3

sem_t mutex;

void thread_routine(void *arg) {
    int tid = *(int *)arg;

    for (int count = 1; count <= NUM_ITERATIONS; count++) {
        sem_down(mutex); // Enter critical section

        printf("Thread %d has entered the critical section (round %d)\n", tid, count);
        usleep(100000); // Simulate critical section work
        printf("Thread %d is leaving the critical section (round %d)\n", tid, count);

        sem_up(mutex); // Exit critical section
        usleep(50000); // Delay before next iteration
    }
}

void start_threads(void *arg __attribute__((unused))) {
    mutex = sem_create(1); // Mutex semaphore

    static int thread_ids[NUM_THREADS] = {101, 102, 103};

    for (int i = 0; i < NUM_THREADS; i++) {
        uthread_create(thread_routine, &thread_ids[i]);
    }
}

int main(void) {
    printf("Starting semaphore mutual exclusion test...\n");
    uthread_run(true, start_threads, NULL);
    sem_destroy(mutex);
    return 0;
}
