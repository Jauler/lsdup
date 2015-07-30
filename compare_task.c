/*
 * Hash and File comparison tasks
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "errno.h"
#include "thread_pool.h"
#include "lf_map.h"
#include "mpmc_lf_queue.h"
#include "list_utils.h"
#include "file_desc.h"
#include "writer.h"

#include "compare_task.h"

#define CT_CMP_CHUNK		1048576

struct comparison_arg {
	struct map *m;
	struct thread_pool *tp;
	struct writer *w;

	struct mpmcq *matchlist;

	struct file_desc *f1;
	struct file_desc *f2;
};

static int ct_enq_writer(struct writer *w, char *f1, char *f2)
{
	//Send matching filenames to writer thread
	int f1_len = strlen(f1);
	int f2_len = strlen(f2);
	char *msg = malloc(f1_len + f2_len + 3);
	memcpy(msg, f1, f1_len);
	msg[f1_len] = ' ';
	memcpy(&msg[f1_len + 1], f2, f2_len);
	msg[f1_len + f2_len + 1] = '\n';
	msg[f1_len + f2_len + 2] = 0;
	return MPMCQ_enqueue(w->wq, msg);
}

void ct_file_worker(void *_arg)
{
	struct comparison_arg *arg = _arg;
	uint8_t *buff1 = NULL, *buff2 = NULL;
	FILE *f1 = NULL, *f2 = NULL;
	int compared_size = 0, chunk_size;

	//Open first file
	f1 = fopen(arg->f1->filename, "r");
	if(f1 == NULL){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		goto CLEANUP;
	}

	//open second file
	f2 = fopen(arg->f2->filename, "r");
	if(f2 == NULL){
		fprintf(stderr, "Error: %s\n", strerror(errno));
		goto CLEANUP;
	}

	//allocate buffers
	buff1 = malloc(CT_CMP_CHUNK);
	if(buff1 == NULL){
		fprintf(stderr, "Error: Out of memory\n");
		goto CLEANUP;
	}
	buff2 = malloc(CT_CMP_CHUNK);
	if(buff2 == NULL){
		fprintf(stderr, "Error: Out of memory\n");
		goto CLEANUP;
	}

	//Read in chunks and compare
	while(compared_size < arg->f1->size){
		//Determine chunk size to be read
		chunk_size = arg->f1->size - compared_size;
		if(chunk_size > CT_CMP_CHUNK)
			chunk_size = CT_CMP_CHUNK;

		//read chunks from files
		if(fread(buff1, chunk_size, 1, f1) != 1){
			fprintf(stderr, "Error reading file %s\n", arg->f1->filename);
			goto CLEANUP;
		}
		if(fread(buff2, chunk_size, 1, f2) != 1){
			fprintf(stderr, "Error reading file %s\n", arg->f1->filename);
			goto CLEANUP;
		}

		//compare buffers
		if(memcmp(buff1, buff2, chunk_size) != 0)
			goto CLEANUP;

		//increment size counter
		compared_size += chunk_size;
	}

	ct_enq_writer(arg->w, arg->f1->filename, arg->f2->filename);

CLEANUP:
	free(buff1);
	free(buff2);
	if(f1 != NULL)
		fclose(f1);
	if(f2 != NULL)
		fclose(f2);
	free(arg);
	return;
}


void ct_hash_worker(void *_arg)
{
	struct comparison_arg *n_arg, *arg = _arg;

	struct mpmcq_elem *base, *trg;
	struct file_desc *base_fd, *trg_fd;
	L_FOREACH(base, arg->matchlist->head.ptr.ptr){
		if(L_DATA(base) == NULL)
			continue;

		L_FOREACH(trg, base){
			//skip empty entries
			if(trg == NULL)
				continue;

			//skip the same files
			if(trg == base)
				continue;

			//get file descriptors
			base_fd = L_DATA(base);
			trg_fd = L_DATA(trg);

			//if both sizes are equal to zero - we treat files as the same in content
			if(base_fd->size == 0 && trg_fd->size == 0){
				ct_enq_writer(arg->w, base_fd->filename, trg_fd->filename);
				continue;
			}

			//Skip files with non-matching hashes
			if(base_fd->hash_valid && trg_fd->hash_valid &&
					memcmp(base_fd->hash, trg_fd->hash, sizeof(base_fd->hash)) != 0)
				continue;

			//allocate memory for a new task
			n_arg = malloc(sizeof(*n_arg));
			if(n_arg == NULL){
				fprintf(stderr, "Out of memory\n");
				continue;
			}

			//fill in fields
			n_arg->m = arg->m;
			n_arg->tp = arg->tp;
			n_arg->w = arg->w;
			n_arg->matchlist = NULL;
			n_arg->f1 = base_fd;
			n_arg->f2 = trg_fd;

			//Enqueue comparison task
			tp_enqueueTask(arg->tp, ct_file_worker, n_arg);
		}
	}

	//Release my arguments
	free(arg);

	return;
}


void ct_enq_worker(void *_arg)
{
	struct comparison_arg *arg = _arg;
	struct comparison_arg *n_arg;

	//check if we have anything in the list
	if(arg->m->ST == NULL)
		return;
	if(arg->m->ST[0] == NULL)
		return;

	//Iterate through list
	struct node *mi;
	struct mpmcq *matchlist; //Potential matches list
	L_FOREACH(mi, arg->m->ST[0][0].ptr.ptr){
		//Get potential matches list
		matchlist = L_DATA(mi);
		if(matchlist == NULL)
			continue;

		//we need at least two files for matching
		if(matchlist->elem_cnt < 2)
			continue;

		//Fill in the fields
		n_arg = malloc(sizeof(*n_arg));
		if(n_arg == NULL){
			fprintf(stderr, "Out of memory\n");
			continue;
		}
		n_arg->m = arg->m;
		n_arg->tp = arg->tp;
		n_arg->w = arg->w;
		n_arg->matchlist = matchlist;

		//enqueue for hash processing
		tp_enqueueTask(arg->tp, ct_hash_worker, n_arg);
	}

	free(arg);

	return;
}


int ct_start(struct thread_pool *tp, struct map *m, struct writer *w)
{
	//Allocate arguments struct
	struct comparison_arg *arg = malloc(sizeof(*arg));
	if(arg == NULL)
		return -ENOMEM;

	//fill in fields
	arg->m = m;
	arg->tp = tp;
	arg->w = w;

	return tp_enqueueTask(tp, ct_enq_worker, arg);
}


