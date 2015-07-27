/*
 * Task for use with thread pool for directory traversing and filling of files map
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>

#include "errno.h"
#include "thread_pool.h"
#include "lf_map.h"
#include "mpmc_lf_queue.h"
#include "list_utils.h"

#include "calc_hash_task.h"

struct cht_enq_files_arg {
	struct map *m;
	struct thread_pool *tp;
};


//Worker for calculating hash values for files
void cht_hash_calc_worker(void *_arg)
{
	//TODO: implement
	return;
}


//Worker for deciding which files must have their hashes calculated
void cht_enq_files_worker(void *_arg)
{
	struct cht_enq_files_arg *arg = _arg;

	//check if we have anything in the list
	if(arg->m->ST == NULL)
		return;
	if(arg->m->ST[0] == NULL)
		return;

	//Iterate through list
	struct node *mi;
	struct mpmcq *matchlist; //Potential matches list
	struct mpmcq_elem *ni;
	L_FOREACH(mi, arg->m->ST[0][0].ptr.ptr){
		//Get potential matches list
		matchlist = L_DATA(mi);
		if(matchlist == NULL)
			continue;

		//Check if we have enough elements for hashes to be useful
		if(matchlist->elem_cnt < CHT_HASH_CALC_THD)
			continue;

		//enqueue tasks for hash calculation
		L_FOREACH(ni, matchlist->head.ptr.ptr){
			if(L_DATA(ni) == NULL)
				continue;

			//Enqueue task for hash calculation
			tp_enqueueTask(arg->tp, cht_hash_calc_worker, L_DATA(ni));
		}
	}

	free(arg);
	return;
}


int cht_start(struct thread_pool *tp, struct map *m)
{
	struct cht_enq_files_arg *arg = malloc(sizeof(*arg));
	if(arg == NULL)
		return -ENOMEM;

	//fill in argument struct
	arg->m = m;
	arg->tp = tp;

	//Enqueue task for thread pool
	return tp_enqueueTask(tp, cht_enq_files_worker, arg);
}


