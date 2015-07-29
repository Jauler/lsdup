/*
 * Writer implementation
 *
 * Author: Rytis Karpuška
 * 		rytis.karpuska@gmail.com
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "errno.h"
#include "mpmc_lf_queue.h"

#include "writer.h"


static void *w_worker(void *arg)
{
	struct writer *w = arg;
	struct timespec ts = {0, 1000000};
	char *msg;

	while(1){
		//Try to get message
		if((msg = MPMCQ_dequeue(w->wq)) == NULL){
			nanosleep(&ts, NULL);
			continue;
		}

		//print and free message
		puts(msg);
		free(msg);
	}

	return 0;
}


struct writer *w_create(void)
{
	//Allocate writer data
	struct writer *w = malloc(sizeof(*w));
	if(w == NULL)
		return NULL;

	//create queue
	w->wq = MPMCQ_create();
	if(w->wq == NULL){
		free(w);
		return NULL;
	}

	//Init thread
	if(pthread_create(&w->pthread, NULL, w_worker, w)){
		free(w->wq);
		free(w);
		return NULL;
	}

	return w;
}


