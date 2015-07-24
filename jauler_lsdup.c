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

#include "errno.h"
#include "thread_pool.h"
#include "lf_map.h"
#include "dir_trav_task.h"

int main(int argc, char *argv[])
{
	printf("Hello World\n");

	struct thread_pool *tp = tp_create(5);
	if(tp == NULL){
		fprintf(stderr, "Could not create thread pool\n");
		return -ENOMEM;
	}

	struct map *m = map_create();
	if(m == NULL){
		fprintf(stderr, "Could not create map\n");
		return -ENOMEM;
	}

	//Traverse directory
	char *path = argc < 2 ? "." : argv[1];
	if(dtt_start(path, tp, m) != 0){
		fprintf(stderr, "Could not traverse directory\n");
		return -EINVAL;
	}

	struct timespec ts = {0, 1000000};
	while(tp->num_enqueued_tasks != 0)
		nanosleep(&ts, NULL);

	return 0;
}
