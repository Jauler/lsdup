/*
 * Lock free hash container implementation
 * Reference: http://www.cs.ucf.edu/~dcm/Teaching/COT4810-Spring2011/Literature/SplitOrderedLists.pdf
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 *
 *
 */

#ifndef LF_HASH_H
#define LF_HASH_H

#include <stdint.h>

#define LF_MAP_SEGMENT_SIZE		512

//TODO list:
//		*second indirection max number of buckets
//		*multiple values support(?)(here or higher levels?)


struct node;

//Define pointer with marker suitable for use in double-word-CAS
#if __x86_64__
union marked_ptr {
	unsigned __int128 blk;
	struct ptr_with_mrk_s {
		struct node *ptr;
		uint32_t mrk;
		uint32_t tag;
	} ptr_mrk;
};
#else
union marked_ptr {
	uint64_t blk;
	struct ptr_with_mrk_s {
		struct node *ptr;
		uint16_t mrk;
		uint16_t tag;
	} ptr_mrk;
};
#endif


struct node {
	void *data;
	uint64_t key;
	union marked_ptr next;
};


struct map {
	union marked_ptr *ST[LF_MAP_SEGMENT_SIZE];
	unsigned int count;
	unsigned int size;
};


/*
 * Create new map instance
 *
 * Return:
 * 		NULL                    - on error
 * 		pointer to struct map * - on success
 */
struct map *map_create(void);


/*
 * Inserts an element into hash
 * NOTE: This hash does support storing multiple elements with the same key
 *
 * Arguments:
 * 		key  - key value
 * 		data - pointer to a data element
 */
int map_add(struct map *m, uint64_t key, void *data);


/*
 * Returns an list of elements with specific key
 *
 * Arguments:
 *		-key  - key value
 *
 * Returns:
 * 		pointer to linked list of elements on success
 * 		NULL on error
 *
 */
void *map_find(struct map *m, uint64_t key);


/*
 * Deletes all elements with a given key
 *
 * Arguments:
 * 		key - key value
 *
 * Returns:
 * 		0                   - on success
 * 		negative error code - on failure
 *
 */
int map_rm(struct map *m, uint64_t key);


//DEBUG tools
void map_print(struct map *m);

#endif

