#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"
#include "queue.h"


/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
#define HZ 100

static struct sigaction orig_action;
static struct itimerval orig_timer;
static bool active = false;
extern queue_t ready_queue;


static void signal_handler(int signum)
{
	//calls uthread to yield

    extern queue_t ready_queue; 
    if (ready_queue && queue_length(ready_queue) > 0) {
        uthread_yield();
    }
}

void preempt_disable(void)
{
	//blocks SIGVTALRM to prevent preemption
	if (!active)
        return;

    sigset_t mask;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGVTALRM);
    
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

void preempt_enable(void)
{
	//enables SIGVTALRM and allows preemption
    if (!active) {
        return;
    }

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGVTALRM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}


void preempt_start(bool preempt)
{
    if (!preempt)
        return;
    
	//if preempt is true, sets a signal handler
    struct sigaction sa;
    struct itimerval timer;

    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    
    sigaction(SIGVTALRM, &sa, &orig_action);
    
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000000 / HZ; 
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000000 / HZ;  
    
	//sets a virtual timer
    setitimer(ITIMER_REAL, &timer, &orig_timer);

    
    active = true;
}

void preempt_stop(void)
{
	if (!active)
        return;

	//restores previous timer
    setitimer(ITIMER_REAL, &orig_timer, NULL);
    sigaction(SIGVTALRM, &orig_action, NULL);
    
    active = false;
}
