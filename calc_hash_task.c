/*
 * Task for use with thread pool for directory traversing and filling of files map
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "errno.h"
#include "thread_pool.h"
#include "lf_map.h"
#include "mpmc_lf_queue.h"
#include "list_utils.h"
#include "file_desc.h"
#include "murmur3_hash.h"

#include "calc_hash_task.h"

#define CHT_HASH_CHUNK			1048576


struct cht_enq_files_arg {
	struct map *m;
	struct thread_pool *tp;
};


//Worker for calculating hash values for files
void cht_hash_calc_worker(void *_arg)
{
	FILE *f;
	struct file_desc *fd = _arg;
	int hashed_size = 0;
	int curr_size;
	uint8_t *buff = malloc(CHT_HASH_CHUNK);
	if(buff == NULL){
		fprintf(stderr, "Error: %s\n", strerror(ENOMEM));
		return;
	}

	//open file
	f = fopen(fd->filename, "r");
	if(f == NULL){
		fprintf(stderr, "Error: %s: %s\n", fd->filename, strerror(errno));
		free(buff);
		return;
	}

	//Read in chunks and calculate hash
	memset(fd->hash, 0, sizeof(fd->hash));
	while(hashed_size < fd->size){
		//Calculate current chunk size
		curr_size = fd->size - hashed_size;
		if(curr_size > CHT_HASH_CHUNK)
			curr_size = CHT_HASH_CHUNK;

		if(fread(buff, curr_size, 1, f) != 1){
			fprintf(stderr, "Error reading file: %s\n", fd->filename);
			goto CLEANUP;
		}

		//Calculate hash of current chunk
		murmur3(buff, curr_size, fd->hash);

		//increment hashed size
		hashed_size += curr_size;
	}

	//Validate hash
	fd->hash_valid = 1;

CLEANUP:
	fclose(f);
	free(buff);
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

		//No need for hashing zero-sized files
		if(L_KEY(mi) == 0)
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


