static void test_main(void *arg)
{
	int threads[NUM_THREADS];
	
	threads[0] = uthread_create(greedy_thread, (void*)0);
	threads[1] = uthread_create(normal_thread, (void*)1);
	threads[2] = uthread_create(normal_thread, (void*)2);
	
	if (threads[0] == -1 || threads[1] == -1 || threads[2] == -1) {
		printf("Error creating threads\n");
		return;
	}

	usleep(100000); /* 100ms */
	
	should_stop = true;
	
	for (int i = 0; i < NUM_THREADS; i++) {
		uthread_join(threads[i], NULL);
	}
	

	printf("\nExecution distribution:\n");
	unsigned long total = 0;
	for (int i = 0; i < NUM_THREADS; i++) {
		total += counters[i];
	}
	
	for (int i = 0; i < NUM_THREADS; i++) {
		printf("Thread %d: %lu iterations (%.2f%%)\n", 
			i, counters[i], (counters[i] * 100.0) / total);
	}
	
	if (counters[0] > 0 && counters[1] > 0 && counters[2] > 0 &&
		counters[1] + counters[2] > counters[0] * 0.1) {  /* Non-greedy threads should get some fair share */
		printf("\nTest PASSED: Preemption works correctly!\n");
	} else {
		printf("\nTest FAILED: Preemption doesn't appear to be working properly\n");
	}
}#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include <uthread.h>
#include <private.h>

#define NUM_THREADS 3
#define ITERATIONS 1000000

static unsigned long counters[NUM_THREADS] = {0};
static volatile bool should_stop = false;

static void test_main(void *arg);

static void greedy_thread(void *arg)
{
	int tid = (int)(long)arg;
	printf("Thread %d starting (greedy)\n", tid);

	while (!should_stop) {
		counters[tid]++;
	}
	
	printf("Thread %d ending with counter = %lu\n", tid, counters[tid]);
}

static void normal_thread(void *arg)
{
	int tid = (int)(long)arg;
	printf("Thread %d starting (normal)\n", tid);
	
	while (!should_stop) {
		counters[tid]++;
		
		if (counters[tid] % 10000 == 0)
			uthread_yield();
	}
	
	printf("Thread %d ending with counter = %lu\n", tid, counters[tid]);
}

int main(void)
{
	uthread_run(true, test_main, NULL);
	
	return 0;
}

static void test_main(void *arg)
{
	int threads[NUM_THREADS];
	
	threads[0] = uthread_create(greedy_thread, (void*)0);
	threads[1] = uthread_create(normal_thread, (void*)1);
	threads[2] = uthread_create(normal_thread, (void*)2);
	
	if (threads[0] == -1 || threads[1] == -1 || threads[2] == -1) {
		printf("Error creating threads\n");
		return;
	}
	
	usleep(100000); /* 100ms */
	
	should_stop = true;
	
	for (int i = 0; i < NUM_THREADS; i++) {
		uthread_join(threads[i], NULL);
	}
	
	printf("\nExecution distribution:\n");
	unsigned long total = 0;
	for (int i = 0; i < NUM_THREADS; i++) {
		total += counters[i];
	}
	
	for (int i = 0; i < NUM_THREADS; i++) {
		printf("Thread %d: %lu iterations (%.2f%%)\n", 
			i, counters[i], (counters[i] * 100.0) / total);
	}
	
	if (counters[0] > 0 && counters[1] > 0 && counters[2] > 0 &&
		counters[1] + counters[2] > counters[0] * 0.1) {  /* Non-greedy threads should get some fair share */
		printf("\nTest PASSED: Preemption works correctly!\n");
	} else {
		printf("\nTest FAILED: Preemption doesn't appear to be working properly\n");
	}
}
