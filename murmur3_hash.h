/*
 * Implementation of murmur64A hash
 *
 * Reference: https://en.wikipedia.org/wiki/MurmurHash
 * Speed reference: http://programmers.stackexchange.com/questions/49550/
 *                  which-hashing-algorithm-is-best-for-uniqueness-and-speed
 *                  http://research.neustar.biz/tag/murmur-hash/
 *
 * Author: Rytis Karpuška
 *			rytis.karpuska@gmail.com
 *			2015
 *
 */

#ifndef MURMUR3_HASH_H
#define MURMUR3_HASH_H

#include <stdint.h>

/*
 * Calculate murmur hash for a data block
 *
 * Arguments:
 * 		data - data block to be hashed
 * 		len  - data block length in bytes
 * 		out  - input seed and output buffer for hash. Has a length of 128bit
 */
void murmur3(const void *data, const int len, uint64_t *out);


#endif

