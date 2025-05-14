#include <stddef.h>
#include <stdlib.h>

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

    if (sem->count == 0) {
        /* No resources available, block current thread */
        struct uthread_tcb *self = uthread_current();
        queue_enqueue(sem->wait, self);
        uthread_block();
        /* When we get here after unblock, preemption is already disabled */
    } else {
        /* Resource available */
        sem->count--;
    }

    preempt_enable();
    return 0;
}

int sem_up(sem_t sem)
{
    if (!sem)
        return -1;

    preempt_disable();

    struct uthread_tcb *next = NULL;
    if (queue_dequeue(sem->wait, (void **)&next) == 0) {
        /* A thread is waiting, unblock it */
        sem->count = 0;  /* Ensure count stays 0 as the thread will consume it */
        uthread_unblock(next);
    } else {
        /* No threads waiting, increment count */
        sem->count++;
    }

    preempt_enable();
    return 0;
}
