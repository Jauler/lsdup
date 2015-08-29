/*
 * Task for use with thread pool for directory traversing and filling of files map
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */


#ifndef DIR_TRAV_TASK_H
#define DIR_TRAV_TASK_H

#include "thread_pool.h"
#include "lf_map.h"


/*
 * Start concurrent Directory Traversing Task
 *
 * This task will traverse directory under *path and will add each file into
 * *map with key value equal to file size
 *
 * Arguments:
 *		path - path to start traversing
 *		tp   - thread pool for concurrency handling
 *		m    - map to add files to
 *
 * Return:
 *		0                   - on success
 *		negative error code - on failure
 */
int dtt_start(char *path, struct thread_pool *tp, struct map *m);


#endif
