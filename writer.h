/*
 * Writer implementation
 *
 * Author: Rytis Karpuška
 * 		rytis.karpuska@gmail.com
 *
 */


#ifndef __WRITER_H
#define __WRITER_H

#include <stdio.h>

#include "pthread.h"
#include "mpmc_lf_queue.h"

struct writer {
	pthread_t pthread;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	struct mpmcq *wq;
	int stop;
	int pause;
	FILE *out;
};


/*
 * Start writer thread
 *
 * Return:
 * 		inited struct writer - on success
 * 		NULL                 - on failure
 */
struct writer *w_create(char *filename);


/*
 * Stop writer and release its resources
 *
 * Note: this is not thread safe, thus calles must take care,
 * that no new enqueues happens while this function is in affect
 *
 * Arguments:
 *		w - writer previsouly returned by w_create
 *
 *	Return:
 *		0       - on success
 *		-EEXIST - if writer queue is not empty
 */
int w_destroy(struct writer *w);


/*
 * Pause writer execution
 *
 * Arguments:
 *		tp - struct thread pool previously returned by tp_create
 *
 */
void w_pause(struct writer *w);

/*
 * Resume threads execution if it was previously paused
 *
 * Arguments:
 *		tp - struct thread pool previously returned by tp_create
 *
 */
void w_resume(struct writer *w);


#endif


