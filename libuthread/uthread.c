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

struct uthread_tcb *current_thread = NULL; // declared globally to use for func uthread_current

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
	struct uthread_tcb *curr = uthread_current();
	struct uthread_tcb *next;

	// change the state from running to ready, add to queue using enqueue
	curr -> thread_state = READY;
	queue_enqueue(ready_queue, curr);
	//next = uthread_ctx_switch(curr, next)
	
	// dequeue next thread in queue, change the state from READY to RUNNING of next thread
    queue_dequeue(ready_queue, (void **)&next);
    next->thread_state = RUNNING; 

    // switch context from the current to the next
    uthread_set_current(next);
    uthread_ctx_switch(&curr->context, &next->context);
}

void uthread_exit(void)
{
	/* TODO Phase 2 */
	struct uthread_tcb *curr = uthread_current();
	curr->thread_state = EXITED;

	if (curr->stack)
		uthread_ctx_destroy_stack(curr->stack);

	free(curr);

	struct uthread_tcb *next;
	if (queue_dequeue(ready_queue, (void**)&next) == 0) {
		next->thread_state = RUNNING;
		current_thread = next;
		setcontext(&next->context); // does not return
	}

	exit(0); // no threads left
}

int uthread_create(uthread_func_t func, void *arg)
{
	//allocate the memory for the new thread 
	struct uthread_tcb *new_thread = malloc(sizeof(struct uthread_tcb));
	if (new_thread == NULL) { // ensure the memory was properly allocated
		return -1;
	}
	
	// allocate stack
	new_thread -> stack = uthread_ctx_alloc_stack();
	if (new_thread -> stack == NULL) { // ensure the memory was properly allocated
		return -1;
	}

	//define thread state
	new_thread -> thread_state = READY;

	//initialize context of the thread using private.h functions
	uthread_ctx_init(&new_thread -> context, new_thread -> stack, func, arg);
	return 0;
}

int uthread_run(bool preempt, uthread_func_t func, void *arg)
{
	/* TODO Phase 2 */
	if (ready_queue != NULL)
		return -1; // prevent reentry

	ready_queue = queue_create();

	// Create the main thread (current context)
	struct uthread_tcb *main_thread = malloc(sizeof(struct uthread_tcb));
	main_thread->thread_state = RUNNING;
	main_thread->stack = NULL;
	getcontext(&main_thread->context);
	current_thread = main_thread;

	// Create the first user thread
	uthread_create(func, arg);

	// Schedule the first thread
	struct uthread_tcb *next;
	queue_dequeue(ready_queue, (void**)&next);
	next->thread_state = RUNNING;
	current_thread = next;
	uthread_ctx_switch(&main_thread->context, &next->context);

	// Cleanup
	free(main_thread);
	return 0;

}

void uthread_block(void)
{
	/* TODO Phase 3 */
}

void uthread_unblock(struct uthread_tcb *uthread)
{
	/* TODO Phase 3 */
}

