/*
 * Thread pool pattern implementation in C
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */


#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

#include <pthread.h>
#include "mpmc_lf_queue.h"

struct thread_pool {
	pthread_t *thread;
	struct mpmcq *wq;
	int num_threads;
	int num_waiting_threads;
	int num_enqueued_tasks;
	int pause;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
};


/*
 * Create a new thread pool
 *
 * Arguments:
 *		num_threads - number of threads in a pool to create
 *
 * Returns:
 *		0                   - on success
 *		negative error code - on failure
 */
struct thread_pool *tp_create(unsigned int num_threads);



/*
 * Enqueue a task for execution.
 *
 * Arguments:
 *		tp   - pointer to struct thread_pool previously returned by tp_create
 *		task - pointer to function of work
 *		arg  - pointer to arguments passed to that function
 *
 * Returns:
 *		0                   - on success
 *		negative error code - on failure
 */
int tp_enqueueTask(struct thread_pool *tp, void (*task)(void *), void *arg);


/*
 * Pause threads execution.
 *
 * Arguments:
 *		tp - struct thread pool previously returned by tp_create
 *
 */
void tp_pause(struct thread_pool *tp);

/*
 * Resume threads execution if it was previusly pused
 *
 * Arguments:
 *		tp - struct thread pool previously returned by tp_create
 *
 */
void tp_resume(struct thread_pool *tp);


#endif


