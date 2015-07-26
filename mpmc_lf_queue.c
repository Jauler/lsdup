/*
 * Implementation of Multiple Producer Multiple Consumer Lock Free queue
 * Reference: http://www.cs.rochester.edu/~scott/papers/1996_PODC_queues.pdf
 *
 * Author: Rytis Karpuška
 *			rytis.karpuska@gmail.com
 *			2015
 *
 */

#include <stdlib.h>
#include <stdint.h>

#include "errno.h"
#include "mpmc_lf_queue.h"

#define CAS(ptr, expected, desired) __atomic_compare_exchange(ptr, \
														expected, \
														desired, \
														0, \
														__ATOMIC_SEQ_CST, \
														__ATOMIC_SEQ_CST)

struct mpmcq *MPMCQ_create(void)
{
	//Allocate structures
	struct mpmcq_elem *node = malloc(sizeof(*node));
	if(node == NULL)
		return NULL;

	struct mpmcq *q = malloc(sizeof(*q));
	if(q == NULL){
		free(node);
		return NULL;
	}

	//Initiate fields
	node->data = NULL;
	node->next.blk = 0;

	//Insert one dummy element
	q->tail.ptr.ptr = node;
	q->tail.ptr.cnt = 0;
	q->head.ptr.ptr = node;
	q->head.ptr.cnt = 0;

	return q;
}


void MPMCQ_destroy(struct mpmcq *q)
{
	//Free all elements from queue
	while(MPMCQ_dequeue(q) != NULL);

	//Free dummy element
	free(q->head.ptr.ptr);

	//Free queue struct
	free(q);

	return;
}


int MPMCQ_enqueue(struct mpmcq *q, void *elem)
{
	union ptr_with_tag tmp;

	//Check if elem is valid
	if(elem == NULL)
		return -EINVAL;

	//Allocate node
	struct mpmcq_elem *node = malloc(sizeof(*node));
	if(node == NULL)
		return -ENOMEM;

	//Initiate elements
	node->data = elem;
	node->next.blk = 0;

	//Try to insert nodes until it has happened
	union ptr_with_tag tail, next;
	while(1){
		tail = q->tail;
		next = tail.ptr.ptr->next;

		//If other thread has inserted, try again
		if(tail.blk != q->tail.blk)
			continue;

		//try to insert new node
		if(next.ptr.ptr == NULL){
			tmp.ptr.ptr = node;
			tmp.ptr.cnt = next.ptr.cnt + 1;
			if(CAS(&tail.ptr.ptr->next.blk, &next.blk, &tmp.blk))
				break;
		} else {
			tmp.ptr.ptr = next.ptr.ptr;
			tmp.ptr.cnt = tail.ptr.cnt + 1;
			CAS(&q->tail.blk, &tail.blk, &tmp.blk);
		}
	}

	//Update tail pointer
	tmp.ptr.ptr = node;
	tmp.ptr.cnt = tail.ptr.cnt + 1;
	CAS(&q->tail.blk, &tail.blk, &tmp.blk);

	return 0;
}

void *MPMCQ_dequeue(struct mpmcq *q)
{
	void *data = NULL;
	union ptr_with_tag head, tail, next, tmp;
	while(1){
		head = q->head;
		tail = q->tail;
		next = head.ptr.ptr->next;

		//If someone else has removed - try again
		if(head.blk != q->head.blk)
			continue;

		//Check for empty and new additions
		//if ok remove element and return its data
		if(head.ptr.ptr == tail.ptr.ptr){
			if(next.ptr.ptr == NULL)
				return NULL;
			tmp.ptr.ptr = next.ptr.ptr;
			tmp.ptr.cnt = tail.ptr.cnt + 1;
			CAS(&q->tail.blk, &tail.blk, &tmp.blk);
		} else {
			data = next.ptr.ptr->data;
			tmp.ptr.ptr = next.ptr.ptr;
			tmp.ptr.cnt = head.ptr.cnt + 1;
			if(CAS(&q->head.blk, &head.blk, &tmp.blk))
				break;
		}
	}

	free(head.ptr.ptr);
	return data;
}





