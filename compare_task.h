/*
 * Hash and File comparison tasks
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */

#ifndef __COMPARE_TASK_H
#define __COMPARE_TASK_H

#include "thread_pool.h"
#include "lf_map.h"


/*
 * Compare hashes and files to figure out if they are the same in content
 *
 * Arguments:
 *		tp - thread pool for task execution
 *		m  - map of potential matches
 *
 * Return:
 *		0                   - on success
 *		negative error code - on failure
 *
 */
int ct_start(struct thread_pool *tp, struct map *m);


#endif

