/*
 * Author: Rytis Karpuška
 * 			rytis.karpuska@gmail.com
 * 			2015
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include "thread_pool.h"
#include "lf_map.h"
#include "dir_trav_task.h"
#include "calc_hash_task.h"
#include "compare_task.h"
#include "free_map_task.h"

struct params {
	int thread_cnt;
	char *scan_path;
};


int scan_params(int argc, char *argv[], struct params *p)
{
	//Default values
	p->thread_cnt = sysconf(_SC_NPROCESSORS_ONLN);
	p->scan_path = ".";

	//Prepare for getopt
	extern char *optarg;
	extern int optind;
	int arg;
	int idx = 1;
	struct option o[3] = {
		{"t", 1, NULL, 'T'},
		{"Threads", 1, NULL, 'T'},
		{0, 0, 0, 0}
	};

	//Scan options
	while((arg = getopt_long_only(argc, argv, "", o, &idx)) != -1){
		switch(arg){
		case 'T':
			p->thread_cnt = atoi(optarg);
			break;

		case '?':
			return -EINVAL;
		}
	}

	//Check if parameters is valid
	if(p->thread_cnt <= 0 || p->thread_cnt >= 100){
		fprintf(stderr, "Invalid thread count. "
				"It must be a number between 1 and 100 inclusive\n");
		return -EINVAL;
	}

	//Check if path is specified as last argument
	if(optind < argc)
		p->scan_path = argv[argc - 1];

	return 0;
}


int main(int argc, char *argv[])
{
	struct timespec ts = {0, 1000000};

	//get program parameters
	struct params p;
	if(scan_params(argc, argv, &p) != 0){
		fprintf(stderr, "Invalid parameters\n");
		return -EINVAL;
	}

	//Create thread pool
	struct thread_pool *tp = tp_create(p.thread_cnt);
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
	if(dtt_start(p.scan_path, tp, m) != 0){
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
	if(ct_start(tp, m) != 0){
		fprintf(stderr, "Could not calculate hashes\n");
		return -EINVAL;
	}

	//Wait for end of comparing
	while(tp->num_enqueued_tasks != 0 ||
			tp->num_waiting_threads != tp->num_threads){
		nanosleep(&ts, NULL);
	}

	//Free potential matches list
	if(fmt_start(tp, m) != 0){
		fprintf(stderr, "Could not free data\n");
		return -EINVAL;
	}

	//Wait for end of freeing
	while(tp->num_enqueued_tasks != 0 ||
			tp->num_waiting_threads != tp->num_threads){
		nanosleep(&ts, NULL);
	}

	//destroy map
	map_destroy(m);

	//destroy thread pool
	tp_destroy(tp);

	return 0;
}
