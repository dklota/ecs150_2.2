#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"

struct node {
	void* data;
	struct node* next;
}; // define a node struct that keeps track of the current node's data + points to the next node

struct queue {
	// learned this in ECS 36C
	int size;
	struct node* head;
	struct node* tail;
};

queue_t queue_create(void) {
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
	if ((queue == NULL) || (data == NULL)) {
		return -1;
	}

	// enqueue = add new node to the current queue, change the tail to point to the new node
	struct node* add_node = malloc(sizeof(struct node)); //allocate memory of the size of the node we want to add
	if (add_node == NULL) {
		return -1; // memory did not allocate correctly
	}

	//init: add with the data and make it the last one by having it point to NULL
	add_node -> data = data;
	add_node -> next = NULL;

	// if condition for if it is the first item in the queue
	if (queue -> size == 0) {
		//add node to the queue - head and tail both point to the new node
		queue -> head = add_node;
		queue -> tail = add_node;
	}
	else {
		//there is more in the queue already
		queue -> tail = add_node;
	}

	queue -> size++; //incr size because we added a node
	return 0;
}

int queue_dequeue(queue_t queue, void **data)
{
	if ((queue == NULL) || (data == NULL) || (queue -> size == 0)) {
		return -1;
	}

	//to dequeue: save the first node + data
	struct node* first_node = queue -> head; // first node is the head of the queue
	*data = first_node -> data; // first node data is data of the head

	// if there is more in the queue - set the head to NULL bc we remove the first item
	// data of the next node becomes the head
	queue -> head = first_node -> next; 
	
	// if the queue is now empty - fix the tail
	if (queue -> head == NULL) {
		queue -> tail == NULL;
	}

	free(first_node);
	queue -> size--; // decr the size
	return 0;
}

int queue_delete(queue_t queue, void *data)
{
	struct node* current = queue -> head;
	struct node* prev = NULL;
	
	while (current != NULL) {
		if (current -> data == data) {
			if (prev == NULL) {
				//deleting head
				queue -> head = current -> next; // if it's first
			}
			else {
				prev -> next = current -> next; // if it's not first
			}

			// delete the tail
			if (current == queue -> tail) {
				queue -> tail = prev;
			}

			free(current);
			queue -> size--;
			return 0;
		}
		current -> data = NULL;
		current = current -> next;
	}
	return -1;
}

int queue_iterate(queue_t queue, queue_func_t func)
{
	if ((queue == NULL) || (func == NULL)) {
		return -1;
	}

	struct node* current = queue -> head; //start at the head node

	while (current != NULL) {
		func(queue, current -> data); // call the function the queue and the data of the current node
		current = current -> next; // incr by making current the next node in the queue 
	}
	return 0;
}

int queue_length(queue_t queue)
{
	return queue -> size; // simply return the size of the queue
}

