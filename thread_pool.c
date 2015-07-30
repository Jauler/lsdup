/*
 * Thread pool pattern implementation in C
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */

#include <stdlib.h>
#include <time.h>

#include "errno.h"
#include "thread_pool.h"
#include "mpmc_lf_queue.h"

struct task {
	void (*task)(void *arg);
	void *arg;
};


static void *thread_worker(void *arg)
{
	struct thread_pool *tp = arg;
	struct task *t;
	struct timespec ts = {0, 1000000}; //1ms
	int waiting = 0;

	while(1){
		//check if we need to pause
		pthread_mutex_lock(&tp->mutex);
		while(tp->pause)
			pthread_cond_wait(&tp->cond, &tp->mutex);
		pthread_mutex_unlock(&tp->mutex);

		//Try to get one work
		if((t = MPMCQ_dequeue(tp->wq)) == NULL){
			//Mark that we are sleeping
			if(!waiting){
				__atomic_add_fetch(&tp->num_waiting_threads, 1, __ATOMIC_SEQ_CST);
				waiting = 1;
			}

			//sleep for a bit
			nanosleep(&ts, NULL);
			continue;
		}
		waiting = 0;

		//Decrement waiting threads count
		__atomic_sub_fetch(&tp->num_waiting_threads, 1, __ATOMIC_SEQ_CST);

		//Execute task
		t->task(t->arg);
		free(t);

		//Decrement task count
		__atomic_sub_fetch(&tp->num_enqueued_tasks, 1, __ATOMIC_SEQ_CST);
	}

	return NULL;
}


struct thread_pool *tp_create(unsigned int num_threads)
{
	int i;

	//Allocate thread_pool structure
	struct thread_pool *tp = malloc(sizeof(*tp));
	if(tp == NULL)
		return NULL;

	//Allocate memory for thread array
	tp->thread = calloc(num_threads, sizeof(*tp->thread));
	if(tp->thread == NULL){
		free(tp);
		return NULL;
	}

	//Create workqueue
	tp->wq = MPMCQ_create();

	//setup state variables
	tp->num_threads = num_threads;
	tp->num_waiting_threads = 0;

	//Init mutex and thread for thread sleeping
	pthread_cond_init(&tp->cond, NULL);
	pthread_mutex_init(&tp->mutex, NULL);
	tp->pause = 0;

	//create actual threads
	for(i = 0; i < num_threads; i++)
		pthread_create(&tp->thread[i], NULL, thread_worker, tp);

	return tp;
}


int tp_enqueueTask(struct thread_pool *tp, void (*task)(void *), void *arg)
{
	int status;
	struct task *t = malloc(sizeof(*t));
	if(t == NULL)
		return -ENOMEM;

	//Setup task variables
	t->task = task;
	t->arg = arg;

	//Enqueue task
	status = MPMCQ_enqueue(tp->wq, t);

	//increment enqueued task count
	if(status == 0)
		__atomic_add_fetch(&tp->num_enqueued_tasks, 1, __ATOMIC_SEQ_CST);

	return status;
}


void tp_pause(struct thread_pool *tp)
{
	pthread_mutex_lock(&tp->mutex);
	tp->pause = 1;
	pthread_mutex_unlock(&tp->mutex);
	return;
}


void tp_resume(struct thread_pool *tp)
{
	pthread_mutex_lock(&tp->mutex);
	tp->pause = 0;
	pthread_cond_broadcast(&tp->cond);
	pthread_mutex_unlock(&tp->mutex);
	return;
}



