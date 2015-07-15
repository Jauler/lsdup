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
#include <stdint.h>

#include "errno.h"
#include "mpsc_lf_queue.h"

#define CAS(ptr, expected, desired) __atomic_compare_exchange(ptr, \
														expected, \
														desired, \
														0, \
														__ATOMIC_SEQ_CST, \
														__ATOMIC_SEQ_CST)

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
	node->next.blk = 0;

	//Insert one dummy element
	q->tail.ptr_cnt.ptr = node;
	q->tail.ptr_cnt.cnt = 0;
	q->head.ptr_cnt.ptr = node;
	q->head.ptr_cnt.cnt = 0;

	return q;
}


void MPSCQ_destroy(struct mpscq *q)
{
	//Free all elements from queue
	while(MPSCQ_dequeue(q) != NULL);

	//Free dummy element
	free(q->head.ptr_cnt.ptr);

	//Free queue struct
	free(q);

	return;
}


int MPSCQ_enqueue(struct mpscq *q, void *elem)
{
	union ptr_with_tag tmp;

	//Check if elem is valid
	if(elem == NULL)
		return -EINVAL;

	//Allocate node
	struct mpscq_elem *node = malloc(sizeof(*node));
	if(node == NULL)
		return -ENOMEM;

	//Initiate elements
	node->data = elem;
	node->next.ptr_cnt.ptr = NULL;

	//Try to insert nodes until it has happened
	union ptr_with_tag tail, next;
	while(1){
		tail = q->tail;
		next = tail.ptr_cnt.ptr->next;

		//If other thread has inserted, try again
		if(tail.blk != q->tail.blk)
			continue;

		//try to insert new node
		if(next.ptr_cnt.ptr == NULL){
			tmp.ptr_cnt.ptr = node;
			tmp.ptr_cnt.cnt = next.ptr_cnt.cnt + 1;
			if(CAS(&tail.ptr_cnt.ptr->next.blk, &next.blk, &tmp.blk))
				break;
		} else {
			tmp.ptr_cnt.ptr = next.ptr_cnt.ptr;
			tmp.ptr_cnt.cnt = tail.ptr_cnt.cnt + 1;
			CAS(&q->tail.blk, &tail.blk, &tmp.blk);
		}
	}

	//Update tail pointer
	tmp.ptr_cnt.ptr = node;
	tmp.ptr_cnt.cnt = tail.ptr_cnt.cnt + 1;
	CAS(&q->tail.blk, &tail.blk, &tmp.blk);

	return 0;
}

void *MPSCQ_dequeue(struct mpscq *q)
{
	void *data = NULL;
	union ptr_with_tag head, tail, next, tmp;
	while(1){
		head = q->head;
		tail = q->tail;
		next = head.ptr_cnt.ptr->next;

		//If someone else has removed - try again
		if(head.blk != q->head.blk)
			continue;

		//Check for empty and new additions
		//if ok remove element and return its data
		if(head.ptr_cnt.ptr == tail.ptr_cnt.ptr){
			if(next.ptr_cnt.ptr == NULL)
				return NULL;
			tmp.ptr_cnt.ptr = next.ptr_cnt.ptr;
			tmp.ptr_cnt.cnt = tail.ptr_cnt.cnt + 1;
			CAS(&q->tail.blk, &tail.blk, &tmp.blk);
		} else {
			data = next.ptr_cnt.ptr->data;
			tmp.ptr_cnt.ptr = next.ptr_cnt.ptr;
			tmp.ptr_cnt.cnt = head.ptr_cnt.cnt + 1;
			if(CAS(&q->head.blk, &head.blk, &tmp.blk));
				break;
		}
	}

	free(head.ptr_cnt.ptr);
	return data;
}





