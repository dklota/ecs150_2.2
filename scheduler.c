#include <stdlib.h>
#include <ucontext.h>
#include <stdio.h>

#include "scheduler.h"


// need to create a ucontext object to use getcontext, makecontext, etc., user needs to create it bc kernel does not know about the scheduler, need to do for both the main thread and task thread bc each has their own TCB
// two context objects: one for main, one for thread (cannot exceed 8 args)
static ucontext_t main_ctx;
static ucontext_t task_ctx[TASK_COUNT_MAX];
static void *task_stack[TASK_COUNT_MAX]; // create a stack of mallocs for each id to free after use

int scheduler_run(void (*task_func)(int), int task_cnt, const int *task_sched)
{

	if (!task_func || !task_sched) {  // ensure that task_func and task_sched exist
    	return -1;
	}
	
	// printf("scheduler_run() called with task_cnt = %d\n", task_cnt);
	if ((task_cnt > TASK_COUNT_MAX) || (task_cnt < 1)) { 
		// printf("scheduler_run: invalid input\n");
		// check to see if number of tasks is greater than task_count_max
		return -1;
	}

	for (int id = 0; id < task_cnt; ++id) {
		// printf("creating task %d\n", id);
		// fflush(stdout);
		getcontext(&task_ctx[id]);

		// printf("got context of task %d\n", id);
		// fflush(stdout);

		if (getcontext(&task_ctx[id]) == -1) {
			return -1;
		}

		// initialize context object for the thread based on ID
		task_stack[id] = malloc(STACK_SIZE);
		if (!task_stack[id]) {
			return -1;
		}
		task_ctx[id].uc_stack.ss_sp = task_stack[id]; // stack for the thread
		task_ctx[id].uc_stack.ss_size = STACK_SIZE; // stack size for the thread
		task_ctx[id].uc_link = &main_ctx;  // return to scheduler if task ends

		makecontext(&task_ctx[id], (void (*)(void))task_func, 1, id);
		// starts at the id of choice
	}

	for (int i = 0; ; i++) {
		int id = task_sched[i]; // current point in the task_scheduler array
		// printf("schedule[%d] = %d\n", i, id);
    	// fflush(stdout);
		if (id == -1) {
			break; // success
		}
		if (id < 0 || id >= task_cnt) {
			return -1;
		}
		swapcontext(&main_ctx, &task_ctx[id]);
	}

	for (int i = 0; i < task_cnt; i++) { // free all the stacks that were allocated per id
		free(task_stack[i]);
	}

	return 0;
}

void scheduler_yield(int task_id)
{
	swapcontext(&task_ctx[task_id], &main_ctx); // swap between the current thread and main thread
}
