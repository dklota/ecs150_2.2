#include <assert.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"
#include "queue.h"

static queue_t ready_queue = NULL;
static struct uthread_tcb *current_thread = NULL; // declared globally to use for the current thread function


enum state { // use enum to define the state
	READY,
	RUNNING,
	BLOCKED,
	EXITED
};

struct uthread_tcb {
	// TCB: thread state, backup of CPU registers, stack
	enum state thread_state; // thread_state
	void* stack; // stack
	int id; // thread id
	uthread_ctx_t context; // context of the thread as provided by private.h
};

struct uthread_tcb *uthread_current(void)
{
	return current_thread;
}

void uthread_yield(void)
{
    struct uthread_tcb *curr = current_thread;
    struct uthread_tcb *next;

    if (queue_length(ready_queue) == 0)
        return;

    curr->thread_state = READY;
    queue_enqueue(ready_queue, curr);

    queue_dequeue(ready_queue, (void **)&next);
    next->thread_state = RUNNING;

    current_thread = next;
    uthread_ctx_switch(&curr->context, &next->context);
}

void uthread_exit(void)
{
	struct uthread_tcb *curr = uthread_current();
	struct uthread_tcb *next;
	curr->thread_state = EXITED;

	if (curr->stack) {
		uthread_ctx_destroy_stack(curr->stack);
	}

	// get next thread before freeing current one
	if (queue_dequeue(ready_queue, (void**)&next) == 0) {
		next->thread_state = RUNNING;
		current_thread = next;
		free(curr);
		// switch to next thread (doesn't return)
		setcontext(&next->context);
	} else {
		// else no threads, so free current and exit
		free(curr);
		exit(0);
	}
}

int uthread_create(uthread_func_t func, void *arg)
{
	//if ready_queue exists
	if (ready_queue == NULL) {
		return 1;
	}

	//allocate the memory for the new thread 
	struct uthread_tcb *new_thread = malloc(sizeof(struct uthread_tcb));
	if (new_thread == NULL) { // ensure the memory was properly allocated
		return -1;
	}
	
	// allocate stack
	new_thread -> stack = uthread_ctx_alloc_stack();
	if (new_thread -> stack == NULL) { // ensure the memory was properly allocated
		free(new_thread);
		return -1;
	}

	//define thread state
	new_thread -> thread_state = READY;

	//initialize context of the thread using private.h functions
	if (uthread_ctx_init(&new_thread->context, new_thread->stack, func, arg) != 0) {
		uthread_ctx_destroy_stack(new_thread->stack);
		free(new_thread);
		return -1;
	}
	
	// add to the ready queue of all threads
	if (queue_enqueue(ready_queue, new_thread) != 0) {
		uthread_ctx_destroy_stack(new_thread->stack);
		free(new_thread);
		return -1;
	}

	return 0;
}

int uthread_run(bool preempt, uthread_func_t func, void *arg)
{
	if (ready_queue != NULL)
		return -1; // prevent reentry

	ready_queue = queue_create();
	// if it's NULL after creation, exit
	if (ready_queue == NULL) {
		return -1;
	}

	// create the main thread = curr
	struct uthread_tcb *main_thread = malloc(sizeof(struct uthread_tcb));
	if (main_thread == NULL) { // check to see if allocation worked
		queue_destroy(ready_queue);
		ready_queue = NULL;
		return -1;
	}

	main_thread->thread_state = RUNNING;
	main_thread->stack = NULL;
	getcontext(&main_thread->context);
	current_thread = main_thread;

	// Create the first user thread, check to see proper memory alloc
	if (uthread_create(func, arg) != 0) {
		free(main_thread);
		queue_destroy(ready_queue);
		ready_queue = NULL;
		return -1;
	}

	// for the threads in the ready_queue
	while (queue_length(ready_queue) > 0) {
		// schedule the next thread, change state and dequeue
		struct uthread_tcb *next;
		queue_dequeue(ready_queue, (void**)&next);
		next->thread_state = RUNNING;
		
		// save current thread for context switch
		struct uthread_tcb *prev = current_thread;
		current_thread = next;
		
		// Switch context to next thread
		uthread_ctx_switch(&prev->context, &next->context);
		
		// When we get back here, check if the previous thread has exited
		if (prev->thread_state == EXITED) {
			if (prev->stack)
				uthread_ctx_destroy_stack(prev->stack);
			free(prev);
		}
	}

	// Cleanup
	free(main_thread);
	queue_destroy(ready_queue);
	return 0;

}

void uthread_block(void)
{
	/* TODO Phase 3 */
	// unblock thread: change the state to blocked
	
}

void uthread_unblock(struct uthread_tcb *uthread)
{
	/* TODO Phase 3 */
}

