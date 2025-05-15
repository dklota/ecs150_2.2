#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "queue.h"
#include "private.h"
#include "sem.h"

struct semaphore {
    size_t count;
    queue_t wait;
};

sem_t sem_create(size_t count)
{
    //this function creates a semaphore and initializes the values 
    sem_t sem = malloc(sizeof(struct semaphore));
    if (!sem)
        return NULL;

    sem->count = count;
    // creates a queue to track waiting threads
    sem->wait = queue_create();
    if (!sem->wait) {
        free(sem);
        return NULL;
    }

    return sem;
}

int sem_destroy(sem_t sem)
{
    // destroys the semaphore and frees objects
    if (!sem || queue_length(sem->wait) > 0)
        // fails if there are still threads waiting
        return -1;

    queue_destroy(sem->wait);
    free(sem);
    return 0;
}

int sem_down(sem_t sem)
{
    //decrements the semaphore count
    if (!sem)
        return -1;

    //avoids race conditions
    preempt_disable();

    
    if (sem->count == 0) {
        struct uthread_tcb *self = uthread_current();
        queue_enqueue(sem->wait, self);
        uthread_block();
    } else {
        sem->count--;
    }

    preempt_enable();
    return 0;
}

int sem_up(sem_t sem)
{
    //increments the semaphore count
    struct uthread_tcb *next;
    
    if (!sem)
        return -1;

    preempt_disable();

    // checks to see if threads are waiting
    if (queue_length(sem->wait) > 0) {
        queue_dequeue(sem->wait, (void **)&next);
        uthread_unblock(next);
    } else {
        sem->count++;
    }

    preempt_enable();
    return 0;
}
