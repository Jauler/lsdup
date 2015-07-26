/*
 * Task for use with thread pool for directory traversing and filling of files map
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

#include "errno.h"
#include "thread_pool.h"
#include "lf_map.h"
#include "dir_trav_task.h"


#define CAS(ptr, expected, desired) __atomic_compare_exchange(ptr, \
														expected, \
														desired, \
														0, \
														__ATOMIC_SEQ_CST, \
														__ATOMIC_SEQ_CST)


struct dtt_arg {
	struct thread_pool *tp;
	struct map *m;
	DIR *dir;
	char path[];
};

void dtt_worker(void *_arg);

static void dtt_handle_file(struct dtt_arg *arg, struct dirent *f)
{
	//TODO: add to map
	return;
}

static void dtt_handle_dir(struct dtt_arg *arg, struct dirent *d)
{
	struct dtt_arg *n_arg;

	if(strcmp(".", d->d_name) == 0)
		return;
	if(strcmp("..", d->d_name) == 0)
		return;

	n_arg = malloc(sizeof(*n_arg) + strlen(arg->path) + strlen(d->d_name) + 2);
	if(n_arg == NULL)
		return;

	n_arg->tp = arg->tp;
	n_arg->m = arg->m;
	n_arg->dir = NULL;
	strcpy(n_arg->path, arg->path);
	if(arg->path[strlen(arg->path) - 1] != '/')
		strcat(n_arg->path, "/");
	strcat(n_arg->path, d->d_name);

	//Enqueue directory reading tasks for all threads
	tp_enqueueTask(n_arg->tp, dtt_worker, n_arg);

	return;
}

void dtt_worker(void *_arg)
{
	struct dtt_arg *arg = _arg;
	struct dirent d_buff, *d;
	int status;

	arg->dir = opendir(arg->path);
	if(arg->dir == NULL){
		fprintf(stderr, "Error: %s: %s\n", arg->path, strerror(errno));
		return;
	}

	while(1){
		//Read entry
		if((status = readdir_r(arg->dir, &d_buff, &d)) != 0)
			break;
		if(d == NULL)
			break;

		//Handle new directory
		if(d->d_type == DT_DIR)
			dtt_handle_dir(arg, d);

		//Handle new file
		if(d->d_type == DT_REG)
			dtt_handle_file(arg, d);
	}

	if(status != 0)
		fprintf(stderr, "Error: %s: %s\n", arg->path, strerror(status));

	closedir(arg->dir);
	free(arg);

	return;
}


int dtt_start(char *path, struct thread_pool *tp, struct map *m)
{
	struct dtt_arg *arg = malloc(sizeof(*arg) + strlen(path) + 1);
	if(arg == NULL)
		return -ENOMEM;

	arg->tp = tp;
	arg->m = m;
	arg->dir = NULL;

	strcpy(arg->path, path);

	//Enqueue directory reading tasks for all threads
	tp_enqueueTask(arg->tp, dtt_worker, arg);


	return 0;
}




