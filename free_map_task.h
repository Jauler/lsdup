/*
 * Free resource held by map
 * No references this time
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 */


#ifndef __FREE_MAP_TASK_H
#define __FREE_MAP_TASK_H

#include "thread_pool.h"
#include "lf_map.h"
#include "mpmc_lf_queue.h"


/*
 * Start map releasing
 *
 * Arguments:
 *		m    - map to add files to
 *
 * Return:
 *		0                   - on success
 *		negative error code - on failure
 */
int fmt_start(struct thread_pool *tp, struct map *m);


#endif
