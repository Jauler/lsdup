/*
 * Implementation of at least Multiple Producer Single Consumer Lock Free queue
 * Reference: http://www.cs.rochester.edu/~scott/papers/1996_PODC_queues.pdf
 *
 * Author: Rytis Karpuška
 *			rytis.karpuska@gmail.com
 *			2015
 *
 */

#ifndef MPSC_LF_QUEUE_H
#define MPSC_LF_QUEUE_H

struct mpscq_elem;

struct mpscq_elem{
	void *data;
	struct mpscq_elem *next;
};

struct mpscq {
	struct mpscq_elem *head;
	struct mpscq_elem *tail;
};


/*
 * Create a new MPSC-LF queue
 *
 * Return:
 * 		NULL                          - if error occured
 * 		pointer to struct mpscq_queue - on success
 */
struct mpscq *MPSCQ_create(void);


/*
 * Destroy a queue previously created with MPSCQ_create
 * NOTE: This function does not free data stored in queue elements
 * NOTE: This function does not support concurrency
 *
 * Arguments:
 * 		q - pointer to queue previously created with MPSCQ_create
 */
void MPSCQ_destroy(struct mpscq *q);


/*
 * Enqeue an element into queue previously created with MPSCQ_create
 *
 * Arguments:
 * 		q    - MPSC queue previously created by MPSCQ_create
 * 		elem - pointer to data
 *
 * Return:
 * 		0                   - on success
 * 		negative error code - on failure
 */
int MPSCQ_enqueue(struct mpscq *q, void *elem);


/*
 * Dequeue an element from queue previously created with MPSCQ_create
 *
 * Arguments:
 * 		q    - MPSC queue previously created by MPSCQ_create
 *
 * Return:
 * 		NULL            - on error
 * 		pointer to data - on success
 */

void *MPSCQ_dequeue(struct mpscq *q);



#endif


