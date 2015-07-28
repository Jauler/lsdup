/*
 * Calculate Hash Task
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */

#ifndef __CALC_HASH_TASK_H
#define __CALC_HASH_TASK_H

#include "lf_map.h"
#include "thread_pool.h"

#define CHT_HASH_CALC_THD			4

/*
 * Calculate hashes of files of the same size
 * In order for hash calculation to proceed there must be at least 4 files
 * of the same size
 *
 * Arguments:
 *		tp - thread pool for task execution
 *		m  - map of files to do a hash calculation on
 *
 * Return:
 *		0                   - on success
 *		negative error code - on failure
 */
int cht_start(struct thread_pool *tp, struct map *m);





#endif

