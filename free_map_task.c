/*
 * Task for use with thread pool for directory traversing and filling of files map
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>

#include "thread_pool.h"
#include "lf_map.h"
#include "mpmc_lf_queue.h"
#include "list_utils.h"
#include "file_desc.h"

#include "free_map_task.h"

struct freeing_arg {
	struct thread_pool *tp;
	struct map *m;
};


void fmt_files_queue_worker(void *arg)
{
	struct mpmcq *q = arg;
	struct file_desc *tmp;

	//Release all data in queue
	while((tmp = MPMCQ_dequeue(q)) != NULL)
		free(tmp);

	//Destroy queue
	MPMCQ_destroy(q);

	return;
}


void fmt_list_traverse_worker(void *_arg)
{
	struct freeing_arg *arg = _arg;

	struct node *tmp, *n = arg->m->ST[0][0].ptr.ptr;
	while(n != NULL){
		tmp = n;
		n = L_NEXT(n);

		//skip null nodes
		if(L_DATA(tmp) == NULL)
			continue;

		tp_enqueueTask(arg->tp, fmt_files_queue_worker, L_DATA(tmp));

		map_rm(arg->m, L_KEY(tmp));
	}

	free(arg);

	return;
}

int fmt_start(struct thread_pool *tp, struct map *m)
{
	//fill in data for worker
	struct freeing_arg *arg = malloc(sizeof(*arg));
	arg->tp = tp;
	arg->m = m;

	return tp_enqueueTask(tp, fmt_list_traverse_worker, arg);
}

