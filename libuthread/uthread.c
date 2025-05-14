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
static int tid_counter = 0; // Thread ID counter for unique IDs

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
	// Get current thread
	struct uthread_tcb *current = uthread_current();
	if (current == NULL || ready_queue == NULL || queue_length(ready_queue) == 0) {
		// Nothing to yield to
		return;
	}

	// Get next thread from ready queue
	struct uthread_tcb *next;
	if (queue_dequeue(ready_queue, (void **)&next) != 0) {
		// No thread to yield to
		return;
	}
	
	// Mark current thread as ready and add to queue
	current->thread_state = READY;
	if (queue_enqueue(ready_queue, current) != 0) {
		// Failed to enqueue current thread, put next thread back and return
		queue_enqueue(ready_queue, next);
		return;
	}
    
	// Mark next thread as running
	next->thread_state = RUNNING;
    
	// Update current thread pointer and perform context switch
	struct uthread_tcb *prev = current;
	current_thread = next;
	
	// Perform context switch
	uthread_ctx_switch(&prev->context, &next->context);
}

void uthread_exit(void)
{
	/* TODO Phase 2 */
	// Get current thread
	struct uthread_tcb *curr = uthread_current();
	if (curr == NULL || ready_queue == NULL) {
		// This should not happen, but just in case
		exit(1);
	}
	
	// Mark current thread as exited
	curr->thread_state = EXITED;

	// Get next thread from ready queue
	struct uthread_tcb *next;
	if (queue_dequeue(ready_queue, (void**)&next) != 0) {
		// No more threads in ready queue, clean up and exit program
		if (curr->stack)
			uthread_ctx_destroy_stack(curr->stack);
		free(curr);
		
		queue_destroy(ready_queue);
		ready_queue = NULL;
		
		exit(0);
	}
	
	// Prepare next thread to run
	next->thread_state = RUNNING;
	current_thread = next;
	
	// We don't free the current thread's resources here
	// They will be freed when control returns to uthread_run
	// after the context switch
	
	// Switch to next thread (doesn't return)
	uthread_ctx_switch(&curr->context, &next->context);
	
	// Should never reach here
	fprintf(stderr, "Error: uthread_exit returned from context switch\n");
	exit(1);
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

	// Define thread state and assign thread ID
	new_thread->thread_state = READY;
	new_thread->id = tid_counter++;

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
	/* TODO Phase 2 */
	// Check if ready_queue already exists
	if (ready_queue != NULL)
		return -1; // prevent reentry

	// Create ready queue
	ready_queue = queue_create();
	if (ready_queue == NULL)
		return -1;

	// Create the main thread (current context)
	struct uthread_tcb *main_thread = malloc(sizeof(struct uthread_tcb));
	if (main_thread == NULL) {
		queue_destroy(ready_queue);
		ready_queue = NULL;
		return -1;
	}
	
	// Initialize main thread
	main_thread->thread_state = RUNNING;
	main_thread->stack = NULL; // Main thread doesn't need a stack allocation
	main_thread->id = tid_counter++;
	if (getcontext(&main_thread->context) != 0) {
		free(main_thread);
		queue_destroy(ready_queue);
		ready_queue = NULL;
		return -1;
	}
	current_thread = main_thread;

	// Create the first user thread
	int ret = uthread_create(func, arg);
	if (ret != 0) {
		free(main_thread);
		queue_destroy(ready_queue);
		ready_queue = NULL;
		return -1;
	}

	// Handle preemption if requested
	if (preempt) {
		// Preemption will be implemented in Phase 4
	}

	// As long as there are threads in the ready queue
	while (queue_length(ready_queue) > 0) {
		// Get next thread from ready queue
		struct uthread_tcb *next_thread;
		if (queue_dequeue(ready_queue, (void**)&next_thread) != 0) {
			// Something went wrong with queue operations
			break;
		}
		
		// Mark next thread as running
		next_thread->thread_state = RUNNING;
		
		// Save previous thread for context switching
		struct uthread_tcb *prev_thread = current_thread;
		
		// Update current thread
		current_thread = next_thread;
		
		// Context switch to next thread
		if (uthread_ctx_switch(&prev_thread->context, &next_thread->context) != 0) {
			// Context switch failed
			fprintf(stderr, "Context switch failed\n");
			break;
		}
		
		// When we return here, check if previous thread has exited
		if (prev_thread->thread_state == EXITED) {
			if (prev_thread->stack != NULL) {
				uthread_ctx_destroy_stack(prev_thread->stack);
			}
			free(prev_thread);
		}
	}

	// Clean up any remaining resources
	queue_destroy(ready_queue);
	ready_queue = NULL;
	
	// If we reach here, all threads have completed
	return 0;
}

void uthread_block(void)
{
	/* TODO Phase 3 */
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
	/* TODO Phase 3 */
	if (uthread == NULL)
		return;
		
	// Only unblock if thread is actually blocked
	if (uthread->thread_state == BLOCKED) {
		uthread->thread_state = READY;
		queue_enqueue(ready_queue, uthread);
	}
}
