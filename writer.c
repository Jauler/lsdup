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
#include <errno.h>
#include <string.h>

#include "errno.h"
#include "mpmc_lf_queue.h"

#include "writer.h"


static void *w_worker(void *arg)
{
	struct writer *w = arg;
	struct timespec ts = {0, 1000000};
	char *msg;

	while(1){
		//check if we need to terminate
		if(w->stop)
			pthread_exit(NULL);

		//check if we need to pause
		pthread_mutex_lock(&w->mutex);
		while(w->pause)
			pthread_cond_wait(&w->cond, &w->mutex);
		pthread_mutex_unlock(&w->mutex);

		//Try to get message
		if((msg = MPMCQ_dequeue(w->wq)) == NULL){
			nanosleep(&ts, NULL);
			continue;
		}

		//print and free message
		fputs(msg, w->out);
		free(msg);
	}

	return 0;
}


struct writer *w_create(char *filename)
{
	//Allocate writer data
	struct writer *w = malloc(sizeof(*w));
	if(w == NULL)
		return NULL;

	//open file
	if((w->out = fopen(filename, "w")) == NULL){
		fprintf(stderr, "Error: %s: %s\n", filename, strerror(errno));
		free(w);
		return NULL;
	}

	//create queue
	w->wq = MPMCQ_create();
	if(w->wq == NULL){
		free(w);
		return NULL;
	}

	//Init mutexes and condition for thread pausing
	pthread_cond_init(&w->cond, NULL);
	pthread_mutex_init(&w->mutex, NULL);
	w->pause = 0;

	//Init thread
	if(pthread_create(&w->pthread, NULL, w_worker, w)){
		free(w->wq);
		free(w);
		return NULL;
	}

	return w;
}


void w_pause(struct writer *w)
{
	pthread_mutex_lock(&w->mutex);
	w->pause = 1;
	pthread_mutex_unlock(&w->mutex);
	return;
}


void w_resume(struct writer *w)
{
	pthread_mutex_lock(&w->mutex);
	w->pause = 0;
	pthread_cond_broadcast(&w->cond);
	pthread_mutex_unlock(&w->mutex);
	return;
}


int w_destroy(struct writer *w)
{
	//check emptyness
	if(w->wq->elem_cnt != 0)
		return -EEXIST;

	//close file
	fclose(w->out);

	//destroy queue
	MPMCQ_destroy(w->wq);

	//stop thread
	w->stop = 1;
	pthread_join(w->pthread, NULL);

	free(w);

	return 0;
}

