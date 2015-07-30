/*
 * Lock free hash container implementation
 * Reference: http://www.cs.ucf.edu/~dcm/Teaching/COT4810-Spring2011/Literature/SplitOrderedLists.pdf
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 *
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "errno.h"
#include "lf_map.h"

static const uint8_t bitReverse256[] = 
{
0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0,
0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8,
0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4,
0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC,
0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2,
0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6,
0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9,
0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3,
0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7,
0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};

#define REVERSE(x)		( \
						(uint64_t)bitReverse256[((x) >>  0) & 0xFF] << 56 | \
						(uint64_t)bitReverse256[((x) >>  8) & 0xFF] << 48 | \
						(uint64_t)bitReverse256[((x) >> 16) & 0xFF] << 40 | \
						(uint64_t)bitReverse256[((x) >> 24) & 0xFF] << 32 | \
						(uint64_t)bitReverse256[((x) >> 32) & 0xFF] << 24 | \
						(uint64_t)bitReverse256[((x) >> 40) & 0xFF] << 16 | \
						(uint64_t)bitReverse256[((x) >> 48) & 0xFF] <<  8 | \
						(uint64_t)bitReverse256[((x) >> 56) & 0xFF] <<  0 \
						)
#define REG_KEY(x)		(REVERSE((x) | 0x8000000000000000ULL))
#define DUM_KEY(x)		(REVERSE(x))

#ifdef __x86_64__
#define MSB_SET(x)				(63 - __builtin_clzl(x))
#else
#define MSB_SET(x)				(63 - __builtin_clzll(x))
#endif

#define GET_PARENT(x)			((x) & ~(1 << MSB_SET(x)))

#define GET_SEGMENT(x)			((x) == 0 ? 0 : MSB_SET(x))
#define GET_SEGMENT_SIZE(x)		((x) == 0 ? 2 : 1 << (x))
#define GET_SEGMENT_OFF(x)		(GET_SEGMENT(x) == 0 ? (x) & 0x01 : \
											(x) & ~(1 << GET_SEGMENT(x)))


#define CAS(ptr, expected, desired) __atomic_compare_exchange(ptr, \
														expected, \
														desired, \
														0, \
														__ATOMIC_SEQ_CST, \
														__ATOMIC_SEQ_CST)


struct srch_status {
	union marked_ptr *prev;
	union marked_ptr cur;
	union marked_ptr next;
};


int l_isInList(struct node *h, uint64_t key, struct srch_status *s)
{
	uint64_t cur_key;
	union marked_ptr cur_0, next_0;
	struct srch_status tmp_s;

	//If s is NULL, just use local version of it
	if(s == NULL)
		s = &tmp_s;

TRY_AGAIN: //Really ugly, but references suggests that....
	s->prev = &h->next;
	s->cur = h->next;

	while(1){
		if(s->cur.ptr.ptr == NULL)
			return 0;

		//save next pointer with mark
		s->next.blk = s->cur.ptr.ptr->next.blk;
		cur_key = s->cur.ptr.ptr->key;

		//Buildup sample unmarked pointer types for comparisons
		cur_0.ptr.ptr = s->cur.ptr.ptr;
		cur_0.ptr.mrk = 0;
		cur_0.ptr.tag = s->cur.ptr.tag;
		next_0.ptr.ptr = s->next.ptr.ptr;
		next_0.ptr.mrk = 0;
		next_0.ptr.tag = s->cur.ptr.tag + 1;

		//Check if insertion did not happen
		if(s->prev->blk != cur_0.blk)
			goto TRY_AGAIN;

		//Check if not marked for deletion
		if(!s->next.ptr.mrk){
			if(cur_key >= key)
				return cur_key == key;
			s->prev = &s->cur.ptr.ptr->next;
		} else {
			if(CAS(&s->prev->blk, &cur_0.blk, &next_0.blk)){
				free(cur_0.ptr.ptr);
				s->next.ptr.tag = s->cur.ptr.tag + 1;
			} else {
				goto TRY_AGAIN;
			}
		}
		s->cur = s->next;
	}

	return 0;
}


int l_insert_with_findres(struct node *h, uint64_t key, uint64_t n_key, void *data,
						struct srch_status *s, struct node **new)
{
	//Allocate node struct
	struct node *n = malloc(sizeof(*n));
	if(n == NULL)
		return -ENOMEM;

	//Fill in data into node
	n->key = key;
	n->n_key = n_key;
	n->data = data;
	n->next.blk = 0;

	union marked_ptr node_ptr;
	while(1){
		//Search for a place to insert our element
		if(l_isInList(h, key, s)){
			free(n);
			return -EEXIST;//TODO: add support for multiple values
		}

		//Set new element next pointer
		n->next.blk = 0;
		n->next.ptr.ptr = s->cur.ptr.ptr;

		//Buildup marked pointer to new element
		node_ptr.ptr.ptr = n;
		node_ptr.ptr.mrk = 0;
		node_ptr.ptr.tag = s->cur.ptr.tag + 1;

		//Try to insert into list
		if(CAS(&s->prev->blk, &s->cur.blk, &node_ptr.blk)){
			if(new != NULL)
				*new = n;
			return 0;
		}
	}

	return 0;
}
int l_insert(struct node *h, uint64_t key, uint64_t n_key, void *data)
{
	struct srch_status s;
	return l_insert_with_findres(h, key, n_key, data, &s, NULL);
}


int l_delete(struct node *h, uint64_t key)
{
	struct srch_status s;

	union marked_ptr tmp[2];
	while(1){
		// Try to find entry
		if(!l_isInList(h, key, &s))
			return -ENOENT;

		// Try to mark node as deleted
		tmp[0].ptr.ptr = s.next.ptr.ptr;
		tmp[0].ptr.mrk = 0;
		tmp[0].ptr.tag = s.next.ptr.tag;
		tmp[1].ptr.ptr = s.next.ptr.ptr;
		tmp[1].ptr.mrk = 1;
		tmp[1].ptr.tag = s.next.ptr.tag + 1;
		if(!CAS(&s.cur.ptr.ptr->next.blk, &tmp[0].blk, &tmp[1].blk))
			continue;

		// Change links of linked list to skip our to-be-deleted node
		tmp[0].ptr.ptr = s.cur.ptr.ptr;
		tmp[0].ptr.mrk = 0;
		tmp[0].ptr.tag = s.cur.ptr.tag;
		tmp[1].ptr.ptr = s.next.ptr.ptr;
		tmp[1].ptr.mrk = 0;
		tmp[1].ptr.tag = s.cur.ptr.tag + 1;
		if(CAS(&s.prev->blk, &tmp[0].blk, &tmp[1].blk))
			free(s.cur.ptr.ptr);
		else
			l_isInList(h, key, &s);

		return 0;
	}

	return 0;
}


struct node *get_bucket(struct map *m, uint64_t bucket_id)
{
	uint64_t segment = GET_SEGMENT(bucket_id);
	if(m->ST[segment] == NULL)
		return NULL;

	uint64_t segment_off = GET_SEGMENT_OFF(bucket_id);
	return m->ST[segment][segment_off].ptr.ptr;
}


int set_bucket(struct map *m, uint64_t bucket_id, struct node *n)
{
	uint64_t segment = GET_SEGMENT(bucket_id);
	struct node *new_segment;
	struct node *null = NULL;
	if(m->ST[segment] == NULL){

		//Allocate new segment
		uint64_t new_cnt = GET_SEGMENT_SIZE(segment);
		new_segment = calloc(new_cnt, sizeof(union marked_ptr));
		if(new_segment == NULL)
			return -EINVAL;

		// Try to set new segment address
		if(!CAS(&m->ST[segment], &null, &new_segment))
			free(new_segment);
	}

	// Save head node
	uint64_t segment_off = GET_SEGMENT_OFF(bucket_id);
	m->ST[segment][segment_off].ptr.ptr = n;
	m->ST[segment][segment_off].ptr.mrk = 0;

	return 0;
}


struct node *init_bucket(struct map *m, uint64_t bucket_id)
{
	//Make sure all parents are also inited
	uint64_t parent = GET_PARENT(bucket_id);
	if(get_bucket(m, parent) == NULL){
		init_bucket(m, parent);
	}

	struct srch_status s;
	struct node *new = NULL;
	if(l_insert_with_findres(get_bucket(m, parent),
					DUM_KEY(bucket_id), LF_MAP_DUMMY_N_KEY,
					NULL, &s, &new) < 0){
		new = s.cur.ptr.ptr;
	}

	//set pointer
	set_bucket(m, bucket_id, new);

	return new;
}


struct map *map_create(void)
{
	//allocate memory for a map
	struct map *m = malloc(sizeof(*m));
	if(m == NULL)
		return NULL;

	//Initiate counters
	m->count = 0;
	m->size = 2;

	//Reset all places in segment table
	memset(m->ST, 0, sizeof(m->ST));

	//Create dummy node for zero bucket
	struct node *n = malloc(sizeof(*n));
	if(n == NULL){
		free(m);
		return NULL;
	}

	//Fill in data
	n->data = NULL;
	n->key = 0;
	n->n_key = LF_MAP_DUMMY_N_KEY;
	n->next.blk = 0;

	//Save dummy node to bucket
	set_bucket(m, 0, n);

	return m;
}


int map_destroy(struct map *m)
{
	if(m->count != 0){
		return -EEXIST;
	}

	//first of all, free all instances of linked list
	struct node *tmp, *n = m->ST[0][0].ptr.ptr;
	while(n != NULL){
		tmp = n;
		n = n->next.ptr.ptr;

		free(tmp);
	}

	//then free all indirection buffers
	int i;
	for(i = 0; i < LF_MAP_SEGMENT_SIZE; i++){
		if(m->ST[i] == NULL)
			continue;

		free(m->ST[i]);
	}

	//release map
	free(m);

	return 0;
}


int map_add(struct map *m, uint64_t key, void *data)
{
	int ret;
	int bucket_id = key % m->size;
	struct node *n = get_bucket(m, bucket_id);

	//If node has not been accesed before
	if(n == NULL){
		n = init_bucket(m, bucket_id);
		if(n == NULL)
			return -ENOMEM;
	}

	//Try to find list
	if((ret = l_insert(n, REG_KEY(key), key, data)) < 0)
		return ret;

	//Take care of hash size
	int curr_size = m->size;
	int d_size = curr_size * 2;
	if(curr_size / __atomic_add_fetch(&m->count, 1, __ATOMIC_SEQ_CST) < 2)
		CAS(&m->size, &curr_size, &d_size);

	return 0;
}


void *map_find(struct map *m, uint64_t key)
{
	int bucket_id = key % m->size;
	struct node *n = get_bucket(m, bucket_id);
	struct srch_status s;

	//If node has not been accesed before
	if(n == NULL){
		n = init_bucket(m, bucket_id);
		if(n == NULL)
			return NULL;
	}

	//Try to find node
	if(!l_isInList(n, REG_KEY(key), &s))
		return NULL;

	return s.cur.ptr.ptr->data;
}


int map_rm(struct map *m, uint64_t key)
{
	int ret;
	int bucket_id = key % m->size;
	struct node *n = get_bucket(m, bucket_id);

	//If node has not been accesed before
	if(n == NULL){
		n = init_bucket(m, bucket_id);
		if(n == NULL)
			return -ENOMEM;
	}

	//Try to delete from bucket
	if((ret = l_delete(n, REG_KEY(key))) < 0)
		return ret;

	//decrement element counter
	__atomic_sub_fetch(&m->count, 1, __ATOMIC_SEQ_CST);
	return 0;
}

#ifdef __x86_64__
void map_print(struct map *m)
{
	int i, j;
	struct node *cur;

	printf("Map:\n");
	printf("  size: %d\n", m->size);
	printf("  count: %d\n", m->count);

	printf("Linked list:\n");
	cur = m->ST[0][0].ptr.ptr;
	while(1){
		//Check if we have reached end of list
		if(cur == NULL){
			printf("NULL\n");
			break;
		}

		//Print key
		printf("[%lx]->", cur->key);

		//Do not overrun lines
		i++;
		if(i % 5 == 0)
			printf("\n");

		//go to next item
		cur = cur->next.ptr.ptr;
	}

	//Print buckets
	printf("Buckets:\n");
	for(i = 0; i < LF_MAP_SEGMENT_SIZE; i++){
		printf("[%i %p]\n", i, m->ST[i]);

		if(m->ST[i] == NULL)
			continue;

		for(j = 0; j < GET_SEGMENT_SIZE(i); j++)
			if(m->ST[i][j].ptr.ptr != NULL)
				printf("    {%d %lx}\n", j, m->ST[i][j].ptr.ptr->key);
			else
				printf("    {%d nil}\n", j);
	}

	return;
}
#else
void map_print(struct map *m)
{
	int i, j;
	struct node *cur;

	printf("Map:\n");
	printf("  size: %d\n", m->size);
	printf("  count: %d\n", m->count);

	printf("Linked list:\n");
	cur = m->ST[0][0].ptr.ptr;
	while(1){
		//Check if we have reached end of list
		if(cur == NULL){
			printf("NULL\n");
			break;
		}

		//Print key
		printf("[%llx]->", cur->key);

		//Do not overrun lines
		i++;
		if(i % 5 == 0)
			printf("\n");

		//go to next item
		cur = cur->next.ptr.ptr;
	}

	//Print buckets
	printf("Buckets:\n");
	for(i = 0; i < LF_MAP_SEGMENT_SIZE; i++){
		printf("[%i %p]\n", i, m->ST[i]);

		if(m->ST[i] == NULL)
			continue;

		for(j = 0; j < GET_SEGMENT_SIZE(i); j++)
			if(m->ST[i][j].ptr.ptr != NULL)
				printf("    {%d %llx}\n", j, m->ST[i][j].ptr.ptr->key);
			else
				printf("    {%d nil}\n", j);
	}

	return;
}
#endif

