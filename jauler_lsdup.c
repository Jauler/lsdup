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

#include "errno.h"
#include "thread_pool.h"
#include "lf_map.h"
#include "dir_trav_task.h"
#include "calc_hash_task.h"
#include "compare_task.h"
#include "writer.h"

#define MAX_LINE			512
static void UI(struct thread_pool *tp, struct writer *w)
{
	struct timeval tv = {0, 0};
	fd_set rfds;
	char buff[MAX_LINE];

	//Check if we have anything on stdin
	FD_ZERO(&rfds);
	FD_SET(0, &rfds);
	if(select(1, &rfds, NULL, NULL, &tv) <= 0)
		return;

	//Read from stdin safely (without overruning buffer)
	int size = read(0, buff, MAX_LINE - 1);

	//just to be sure our string does not overrun
	buff[size] = 0;

	if(strcmp("start\n", buff) == 0){
		tp_resume(tp);
		w_resume(w);
	}

	if(strcmp("stop\n", buff) == 0){
		tp_pause(tp);
		w_pause(w);
	}


	return;
}


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
		UI(tp, w);
		nanosleep(&ts, NULL);
	}

	//Calculate hashes of potential matches
	if(cht_start(tp, m) != 0){
		fprintf(stderr, "Could not calculate hashes\n");
		return -EINVAL;
	}

	//Wait for end of hashing
	while(tp->num_enqueued_tasks != 0){
		UI(tp, w);
		nanosleep(&ts, NULL);
	}

	//Calculate hashes of potential matches
	if(ct_start(tp, m, w) != 0){
		fprintf(stderr, "Could not calculate hashes\n");
		return -EINVAL;
	}

	//Wait for end of hashing
	while(tp->num_enqueued_tasks != 0 || w->wq->elem_cnt != 0){
		UI(tp, w);
		nanosleep(&ts, NULL);
	}

	return 0;
}
