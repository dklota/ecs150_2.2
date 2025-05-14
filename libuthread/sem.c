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

    preempt_disable();  // Use preempt_disable instead

    if (sem->count > 0) {
        sem->count--;
    } else {
        struct uthread_tcb *self = uthread_current();
        queue_enqueue(sem->wait, self);
        uthread_block();
        preempt_enable();  // Use preempt_enable instead
        return sem_down(sem);  // Try again after being unblocked
    }

    preempt_enable();  // Use preempt_enable instead
    return 0;
}

int sem_up(sem_t sem)
{
    if (!sem)
        return -1;

    preempt_disable();  // Use preempt_disable instead

    struct uthread_tcb *next;
    if (queue_dequeue(sem->wait, (void **)&next) == 0) {
        uthread_unblock(next);
    } else {
        sem->count++;
    }

    preempt_enable();  // Use preempt_enable instead
    return 0;
}
