/*
 * Author: Rytis Karpuška
 * 			rytis.karpuska@gmail.com
 * 			2015
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/select.h>
#include <errno.h>

#include "thread_pool.h"
#include "lf_map.h"
#include "dir_trav_task.h"
#include "calc_hash_task.h"
#include "compare_task.h"
#include "free_map_task.h"
#include "writer.h"


#define MAX_LINE			512

int main(int argc, char *argv[])
{
	struct timespec ts = {0, 1000000};

	//parse user args
	if(argc != 4){
		fprintf(stderr, "Usage: %s <scan directory> <output filename> "
									"<number of search threads>\n", argv[0]);
		return 0;
	}
	char *dir = argv[1];
	char *out = argv[2];
	int cnt = atoi(argv[3]);
	if(cnt <= 0 || cnt >= 100){
		fprintf(stderr, "Supported thread count is between 1 and 99 inclusive\n");
		return 0;
	}

	//create writer
	struct writer *w = w_create(out);
	if(w == NULL){
		fprintf(stderr, "Could not create writer\n");
		return -ENOMEM;
	}

	//Create thread pool
	struct thread_pool *tp = tp_create(cnt);
	if(tp == NULL){
		fprintf(stderr, "Could not create thread pool\n");
		return -ENOMEM;
	}

	//Create empty map of potential matches by size
	struct map *m = map_create();
	if(m == NULL){
		fprintf(stderr, "Could not create map\n");
		return -ENOMEM;
	}

	//Traverse directory
	char *path = argc < 2 ? "." : dir;
	if(dtt_start(path, tp, m, out) != 0){
		fprintf(stderr, "Could not traverse directory\n");
		return -EINVAL;
	}

	//Wait for end of traversing
	while(tp->num_enqueued_tasks != 0){
		nanosleep(&ts, NULL);
	}

	//Calculate hashes of potential matches
	if(cht_start(tp, m) != 0){
		fprintf(stderr, "Could not calculate hashes\n");
		return -EINVAL;
	}

	//Wait for end of hashing
	while(tp->num_enqueued_tasks != 0){
		nanosleep(&ts, NULL);
	}

	//Calculate hashes of potential matches
	if(ct_start(tp, m, w) != 0){
		fprintf(stderr, "Could not calculate hashes\n");
		return -EINVAL;
	}

	//Wait for end of comparing
	while(tp->num_enqueued_tasks != 0 ||
			tp->num_waiting_threads != tp->num_threads ||
			w->wq->elem_cnt != 0){
		nanosleep(&ts, NULL);
	}

	//Free potential matches list
	if(fmt_start(tp, m) != 0){
		fprintf(stderr, "Could not free data\n");
		return -EINVAL;
	}

	//Wait for end of freeing
	while(tp->num_enqueued_tasks != 0 ||
			tp->num_waiting_threads != tp->num_threads ||
			w->wq->elem_cnt != 0){
		nanosleep(&ts, NULL);
	}

	//destroy map
	map_destroy(m);

	//destroy thread pool
	tp_destroy(tp);

	//Destroy writer
	w_destroy(w);

	return 0;
}
