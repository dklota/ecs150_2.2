#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "private.h"
#include "uthread.h"

/*
 * Frequency of preemption
 * 100Hz is 100 times per second
 */
#define HZ 100

static struct sigaction original_action;
static struct itimerval original_timer;
static bool preemption_active = false;

static void signal_handler(int signum)
{
    uthread_yield();
}

void preempt_disable(void)
{
	if (!preemption_active)
        return;

    sigset_t mask;
    
    sigemptyset(&mask);
    sigaddset(&mask, SIGVTALRM);
    
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

void preempt_enable(void)
{
	if (!preemption_active)
        return;

    sigset_t mask;

    sigemptyset(&mask);
    sigaddset(&mask, SIGVTALRM);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

void preempt_start(bool preempt)
{
    if (!preempt)
        return;
    
    struct sigaction sa;
    struct itimerval timer;
    
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    
    sigaction(SIGVTALRM, &sa, &original_action);
    
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 1000000 / HZ; 
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 1000000 / HZ;  
    
    setitimer(ITIMER_VIRTUAL, &timer, &original_timer);
    
    preemption_active = true;
}

void preempt_stop(void)
{
	if (!preemption_active)
        return;
    
    setitimer(ITIMER_VIRTUAL, &original_timer, NULL);
    sigaction(SIGVTALRM, &original_action, NULL);
    
    preemption_active = false;
}

