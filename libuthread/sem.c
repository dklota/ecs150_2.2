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
    sem_t sem = malloc(sizeof(struct semaphore));
    if (!sem)
        return NULL;

    sem->count = count;
    sem->wait = queue_create();
    if (!sem->wait) {
        free(sem);
        return NULL;
    }

    return sem;
}

int sem_destroy(sem_t sem)
{
    if (!sem || queue_length(sem->wait) > 0)
        return -1;

    queue_destroy(sem->wait);
    free(sem);
    return 0;
}

int sem_down(sem_t sem)
{
    if (!sem)
        return -1;

    preempt_disable();

    /* If count is 0, block until a resource becomes available */
    if (sem->count == 0) {
        struct uthread_tcb *self = uthread_current();
        queue_enqueue(sem->wait, self);
        uthread_block();
    } else {
        /* Resource available */
        sem->count--;
    }

    preempt_enable();
    return 0;
}

int sem_up(sem_t sem)
{
    struct uthread_tcb *next;
    
    if (!sem)
        return -1;

    preempt_disable();

    /* If there are blocked threads, wake up one */
    if (queue_length(sem->wait) > 0) {
        queue_dequeue(sem->wait, (void **)&next);
        /* We decrease count below, so don't need to adjust it here */
        uthread_unblock(next);
    } else {
        /* No waiting threads, increment count */
        sem->count++;
    }

    preempt_enable();
    return 0;
}
