#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

struct node {
	void* data;
	struct node* next;
}; // define a node struct that keeps track of the current node's data + points to the next node

struct queue {
	/* TODO Phase 1 */
	// learned this in ECS 36C
	int size;
	struct node* head;
	struct node* tail;
};

queue_t queue_create(void)
{
	/* TODO Phase 1 */
	queue_t q = malloc(sizeof(struct queue)); //using malloc to allocate memory of the size of the struct 
	q -> head = NULL;
	q -> tail = NULL;
	q -> size = 0;

	return q;
}

int queue_destroy(queue_t queue)
{
	/* TODO Phase 1 */

	if (queue == NULL) {
		return -1;
	}
	free(queue); // deallocate everything and free the memory to avoid memory leaks
	queue = NULL;
	return 0;
}

int queue_enqueue(queue_t queue, void *data)
{
	/* TODO Phase 1 */
	if ((queue == NULL) || (data == NULL)) {
		return -1;
	}

	// enqueue = add new node to the current queue, change the tail to point to the new node
	struct node* add_node = malloc(sizeof(struct node)); //allocate memory of the size of the node we want to add
	if (add_node == NULL) {
		return -1; // memory did not allocate correctly
	}

	add_node -> data = data;
	add_node -> next = NULL;

	//TO-DO: add node to the queue

}

int queue_dequeue(queue_t queue, void **data)
{
	/* TODO Phase 1 */
}

int queue_delete(queue_t queue, void *data)
{
	/* TODO Phase 1 */
}

int queue_iterate(queue_t queue, queue_func_t func)
{
	/* TODO Phase 1 */
}

int queue_length(queue_t queue)
{
	/* TODO Phase 1 */
}

