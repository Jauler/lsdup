/*
 * Implementation of at least Multiple Producer Single Consumer Lock Free queue
 * Reference: http://www.cs.rochester.edu/~scott/papers/1996_PODC_queues.pdf
 *
 * Author: Rytis Karpuška
 *			rytis.karpuska@gmail.com
 *			2015
 *
 */

#include <stdlib.h>
#include <errno.h>

#include "mpsc_lf_queue.h"


struct mpscq *MPSCQ_create(void)
{
	//Allocate structures
	struct mpscq_elem *node = malloc(sizeof(*node));
	if(node == NULL)
		return NULL;

	struct mpscq *q = malloc(sizeof(*q));
	if(q == NULL){
		free(node);
		return NULL;
	}

	//Initiate fields
	node->data = NULL;
	node->next = NULL;

	//Insert one dummy element
	q->tail = node;
	q->head = node;

	return q;
}


void MPSCQ_destroy(struct mpscq *q)
{
	//Free all elements from queue
	while(MPSCQ_dequeue(q) != NULL);

	//Free dummy element
	free(q->head);

	//Free queue struct
	free(q);

	return;
}


int MPSCQ_enqueue(struct mpscq *q, void *elem)
{
	//Allocate node
	struct mpscq_elem *node = malloc(sizeof(*node));
	if(node == NULL)
		return -ENOMEM;

	//Initiate elements
	node->data = elem;
	node->next = NULL;

	//Try to insert nodes until it has happened
	struct mpscq_elem *tail, *next;
	while(1){
		tail = q->tail;
		next = tail->next;

		//If other thread has inserted, try again
		if(tail != q->tail)
			continue;

		//try to insert new node
		if(next == NULL){
			if(__sync_bool_compare_and_swap(&tail->next, next, node))
				break;
		} else {
			__sync_bool_compare_and_swap(&q->tail, tail, next);
		}
	}

	//Update tail pointer
	__sync_bool_compare_and_swap(&q->tail, tail, node);

	return 0;
}

void *MPSCQ_dequeue(struct mpscq *q)
{
	void *data = NULL;
	struct mpscq_elem *head, *tail, *next;
	while(1){
		head = q->head;
		tail = q->tail;
		next = head->next;

		//If someone else has removed - try again
		if(head != q->head)
			continue;

		//Check for empty and new additions
		//if ok remove element and return its data
		if(head == tail){
			if(next == NULL)
				return NULL;
			__sync_bool_compare_and_swap(&q->tail, tail, next);
		} else {
			data = next->data;
			if(__sync_bool_compare_and_swap(&q->head, head, next));
				break;
		}
	}

	free(head);
	return data;
}





