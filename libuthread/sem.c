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

	if (sem->count > 0) {
        sem->count--;
    } else {
        uthread_t self = uthread_current();
        queue_enqueue(sem->wait_queue, self);
        uthread_block();
        return sem_down(sem);
    }

    return 0;
}

int sem_up(sem_t sem)
{
	if (!sem)
        return -1;

    uthread_t next;
    if (queue_dequeue(sem->wait_queue, (void **)&next) == 0) {
        uthread_unblock(next);
    } else {
        sem->count++;
    }

    return 0;
}

