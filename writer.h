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
	struct mpmcq *wq;
};


/*
 * Start writer thread
 *
 * Return:
 * 		inited struct writer - on success
 * 		NULL                 - on failure
 */
struct writer *w_create(void);

/*
 * Enqueue message to be printed
 *
 * Arguments:
 * 		w      - struct writer previously inited by w_start
 *		stream - stdout, stderr, or open FILE *
 *		msg    - message to print
 */
int w_enqueue(struct writer *w, FILE *stream, char *msg);



#endif


