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
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "thread_pool.h"
#include "lf_map.h"
#include "mpmc_lf_queue.h"
#include "dir_trav_task.h"
#include "file_desc.h"


struct dtt_arg {
	struct thread_pool *tp;
	struct map *m;
	char *out;
	DIR *dir;
	char path[];
};


void dtt_worker(void *_arg);


static void dtt_handle_file(struct dtt_arg *arg, struct dirent *f)
{
	//get memory for a pathname
	int off = 0;
	int pathname_len = strlen(arg->path);
	int filename_len = strlen(f->d_name);
	struct file_desc *fd = calloc(1, sizeof(*fd) + pathname_len + filename_len + 2);
	if(fd ==  NULL){
		fprintf(stderr, "Error: %s\n", strerror(ENOMEM));
		return;
	}

	//format full pathname
	memcpy(fd->filename, arg->path, pathname_len);
	if(arg->path[pathname_len - 1] != '/'){
		fd->filename[pathname_len] = '/';
		off = 1;
	}
	memcpy(fd->filename + pathname_len + off, f->d_name, filename_len);
	fd->filename[pathname_len + off + filename_len] = 0;

	//stat the file to figure out its size
	struct stat fs;
	if(stat(fd->filename, &fs) != 0){
		fprintf(stderr, "Error: %s: %s\n", fd->filename, strerror(errno));
		free(fd);
		return;
	}

	fd->size = fs.st_size;

	//try to find a file with the same size
	void *eq_sz_list;
	if((eq_sz_list = map_find(arg->m, fs.st_size)) == NULL){
		eq_sz_list = MPMCQ_create();
		map_add(arg->m, fs.st_size, eq_sz_list);
	}

	//enqueue file
	MPMCQ_enqueue(eq_sz_list, fd);

	return;
}


static void dtt_handle_dir(struct dtt_arg *arg, struct dirent *d)
{
	struct dtt_arg *n_arg;

	//allocate buffer for new taskarg. Also include string sizes
	n_arg = malloc(sizeof(*n_arg) + strlen(arg->path) + strlen(d->d_name) + 2);
	if(n_arg == NULL)
		return;

	//fill in taskarg struct
	n_arg->tp = arg->tp;
	n_arg->m = arg->m;
	n_arg->out = arg->out;
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

		if(strcmp(".", d->d_name) == 0)
			continue;
		if(strcmp("..", d->d_name) == 0)
			continue;
		if(strcmp(arg->out, d->d_name) == 0)
			continue;


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


int dtt_start(char *path, struct thread_pool *tp, struct map *m, char *out)
{
	struct dtt_arg *arg = malloc(sizeof(*arg) + strlen(path) + 1);
	if(arg == NULL)
		return -ENOMEM;

	//Fill in data
	arg->tp = tp;
	arg->m = m;
	arg->out = out;
	arg->dir = NULL;
	strcpy(arg->path, path);

	//Enqueue directory reading tasks for all threads
	tp_enqueueTask(arg->tp, dtt_worker, arg);

	return 0;
}




