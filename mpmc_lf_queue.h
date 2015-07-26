/*
 * Implementation of Multiple Producer Multiple Consumer Lock Free queue
 * Reference: http://www.cs.rochester.edu/~scott/papers/1996_PODC_queues.pdf
 *
 * Author: Rytis Karpuška
 *			rytis.karpuska@gmail.com
 *			2015
 *
 */

#ifndef MPMC_LF_QUEUE_H
#define MPMC_LF_QUEUE_H

#include <stdint.h>

struct mpmcq_elem;

//Define pointer with tag structure for ABA-problem solution
#if __x86_64__
union ptr_with_tag {
	unsigned __int128 blk;
	struct ptr_with_cnt_s {
		struct mpmcq_elem *ptr;
		uint64_t cnt;
	} ptr;

};
#else
union ptr_with_tag {
	uint64_t blk;
	struct ptr_with_cnt_s {
		struct mpmcq_elem *ptr;
		uint32_t cnt;
	} ptr;
};
#endif


struct mpmcq_elem{
	void *data;
	union ptr_with_tag next;
};

struct mpmcq {
	union ptr_with_tag head;
	union ptr_with_tag tail;
};


/*
 * Create a new MPMC-LF queue
 *
 * Return:
 * 		NULL                          - if error occured
 * 		pointer to struct mpmcq_queue - on success
 */
struct mpmcq *MPMCQ_create(void);


/*
 * Destroy a queue previously created with MPMCQ_create
 * NOTE: This function does not free data stored in queue elements
 * NOTE: This function does not support concurrency
 *
 * Arguments:
 * 		q - pointer to queue previously created with MPMCQ_create
 */
void MPMCQ_destroy(struct mpmcq *q);


/*
 * Enqeue an element into queue previously created with MPMCQ_create
 *
 * Arguments:
 * 		q    - MPMC queue previously created by MPMCQ_create
 * 		elem - pointer to data
 *
 * Return:
 * 		0                   - on success
 * 		negative error code - on failure
 */
int MPMCQ_enqueue(struct mpmcq *q, void *elem);


/*
 * Dequeue an element from queue previously created with MPMCQ_create
 *
 * Arguments:
 * 		q    - MPMC queue previously created by MPMCQ_create
 *
 * Return:
 * 		NULL            - on error
 * 		pointer to data - on success
 */

void *MPMCQ_dequeue(struct mpmcq *q);



#endif


