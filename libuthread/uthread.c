
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
	/* TODO Phase 2/3 */
	return current_thread;
}

void uthread_yield(void)
{
	/* TODO Phase 2 */
	// define the current thread and next thread
	// the goal is to allow the next thread to execute and stop in the current thread
	struct uthread_tcb *curr = uthread_current();
	struct uthread_tcb *next;

	//printf("[yield] current thread: %p\n", curr);

	// if (queue_length(ready_queue) == 0) {
	// 	printf("[yield] ready_queue is empty, skipping yield\n");
	// 	return;
	// }

	if (!curr || !ready_queue) {
       //fprintf(stderr, "[yield] Error: current thread or ready queue is NULL\n");
        return;
    }

	//printf("[yield] current thread: %p\n", curr);

	// change the state of the current thread to ready and add it to the end of the ready_queue
	curr->thread_state = READY;
	//queue_enqueue(ready_queue, curr);
	if (queue_enqueue(ready_queue, curr) < 0) {
        //fprintf(stderr, "[yield] Error: failed to enqueue current thread\n");
        return;
    }

	//printf("[yield] enqueued current thread: %p\n", curr);

	// Check if dequeue succeeded
	//printf("[yield] attempting to dequeue next thread\n");
	if (queue_dequeue(ready_queue, (void **)&next) != 0 || next == NULL) {
        //fprintf(stderr, "[yield] Error: no thread to yield to — ready queue empty or dequeue failed\n");
        return;
    }

	// Prepare to switch to next thread
    next->thread_state = RUNNING;
    current_thread = next;
    //printf("[yield] switching to next thread: %p\n", next);

    // Confirm before switching
    //printf("[yield] About to switch: curr=%p, next=%p\n", curr, next);
    uthread_ctx_switch(&curr->context, &next->context);
}

void uthread_exit(void)
{
	/* TODO Phase 2 */
	struct uthread_tcb *curr = uthread_current();
	struct uthread_tcb *next;
	
	curr->thread_state = EXITED;

	if (curr->stack)
		uthread_ctx_destroy_stack(curr->stack);

	// Get next thread before freeing current one
	if (queue_dequeue(ready_queue, (void**)&next) == 0) {
		next->thread_state = RUNNING;
		current_thread = next;
		
		// Free current thread after getting next thread
		free(curr);
		
		// Switch to next thread (doesn't return)
		setcontext(&next->context);
	} else {
		// No more threads, free current and exit
		free(curr);
		exit(0);
	}
}

int uthread_create(uthread_func_t func, void *arg)
{
	/* TODO Phase 2 */
	// Check if ready_queue exists
	if (ready_queue == NULL)
		return -1;
		
	// Allocate the memory for the new thread
	struct uthread_tcb *new_thread = malloc(sizeof(struct uthread_tcb));
	if (new_thread == NULL) { // ensure the memory was properly allocated
		return -1;
	}
	
	// Allocate stack
	new_thread->stack = uthread_ctx_alloc_stack();
	if (new_thread->stack == NULL) { // ensure the memory was properly allocated
		free(new_thread);
		return -1;
	}

	// Define thread state
	new_thread->thread_state = READY;

	// Initialize context of the thread using private.h functions
	if (uthread_ctx_init(&new_thread->context, new_thread->stack, func, arg) != 0) {
		uthread_ctx_destroy_stack(new_thread->stack);
		free(new_thread);
		return -1;
	}
	
	// Add to ready queue
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

	// Create ready queue
	ready_queue = queue_create();
	if (ready_queue == NULL)
		return -1;

	// run preemption start
    //preempt_start(preempt);

	// Create the main thread (current context)
	struct uthread_tcb *main_thread = malloc(sizeof(struct uthread_tcb));
	if (main_thread == NULL) {
		queue_destroy(ready_queue);
		ready_queue = NULL;
		preempt_stop();
		return -1;
	}
	
	main_thread->thread_state = RUNNING;
	main_thread->stack = NULL;
	getcontext(&main_thread->context);
	current_thread = main_thread;

	// Create the first user thread
	if (uthread_create(func, arg) != 0) {
		free(main_thread);
		queue_destroy(ready_queue);
		ready_queue = NULL;
		preempt_stop();
		return -1;
	}

	// Handle preemption if requested
	if (preempt) {
		preempt_start(preempt);
	}

	// While there are threads in the ready queue
	while (queue_length(ready_queue) > 0) {
		// Schedule the next thread
		struct uthread_tcb *next;
		queue_dequeue(ready_queue, (void**)&next);
		next->thread_state = RUNNING;
		
		// Save current thread for context switch
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

	// Cleanup main thread and ready queue
	if (preempt) {
        preempt_stop();
    } 
	free(main_thread);
	queue_destroy(ready_queue);
	ready_queue = NULL;
	
	return 0;
}

void uthread_block(void)
{
	struct uthread_tcb *curr = uthread_current();
	struct uthread_tcb *next;
	
	// Set current thread state to BLOCKED (don't add to ready queue)
	curr->thread_state = BLOCKED;
	
	// Get next thread from ready queue
	if (queue_dequeue(ready_queue, (void**)&next) == 0) {
		next->thread_state = RUNNING;
		current_thread = next;
		uthread_ctx_switch(&curr->context, &next->context);
	} else {
		// No threads in ready queue - this shouldn't happen in normal operation
		fprintf(stderr, "Error: No threads ready to run after block\n");
		exit(1);
	}
}

void uthread_unblock(struct uthread_tcb *uthread)
{
	if (uthread == NULL)
		return;
		
	// Only unblock if thread is actually blocked
	if (uthread->thread_state == BLOCKED) {
		uthread->thread_state = READY;
		queue_enqueue(ready_queue, uthread);
	}
}
