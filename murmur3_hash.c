/*
 * Implementation of murmur64A hash
 *
 * Reference: https://en.wikipedia.org/wiki/MurmurHash
 * Speed reference: http://programmers.stackexchange.com/questions/49550/
 *                  which-hashing-algorithm-is-best-for-uniqueness-and-speed
 *
 * Author: Rytis Karpuška
 *			rytis.karpuska@gmail.com
 *			2015
 *
 */

#include <stdint.h>

#include "murmur3_hash.h"


//Helper rotate function
static inline uint64_t rotl64(uint64_t x, int8_t n)
{
	//Normaly gcc should optimize this to rotl instruction
	return (x << n) | (x >> (64 - n));
}

static inline uint64_t fmix64(uint64_t k)
{
	k ^= k >> 33;
	k *= 0xff51afd7ed558ccdULL;
	k ^= k >> 33;
	k *= 0xc4ceb9fe1a85ec53ULL;
	k ^= k >> 33;
	return k;
}



void murmur3(const void *data, const int len, uint64_t *out)
{
	int i;
	uint64_t k1, k2;

	//Initiate seed and constants
	uint64_t h1 = out[0];
	uint64_t h2 = out[2];
	uint64_t c1 = 0x87c37b91114253d5ULL;
	uint64_t c2 = 0x4cf5ad432745937fULL;

	//Do Main calculation
	int blkCnt = len / 16;
	uint64_t *buf64 = (uint64_t *)data;
	for(i = 0; i < blkCnt; i++){
		//Retreive data from buffer
		k1 = buf64[i*2 + 0];
		k2 = buf64[i*2 + 1];

		k1 *= c1;
		k1  = rotl64(k1, 31);
		k1 *= c2;
		h1 ^= k1;

		h1 = rotl64(h1, 27);
		h1 += h2;
		h1 = h1*5+0x52dce729;

		k2 *= c2;
		k2  = rotl64(k2, 33);
		k2 *= c1;
		h2 ^= k2;

		h2 = rotl64(h2, 31);
		h2 += h1;
		h2 = h2*5+0x38495ab5;
	}

	//Deal with unaligned bytes
	//NOTE: there should be a smarter way for dealing with this
	uint8_t *buf8 = (uint8_t *)((uint8_t *)data + blkCnt*16);
	k1 = 0;
	k2 = 0;
	switch(len & 0xF){
	case 15:
		k2 ^= ((uint64_t)buf8[14]) << 48;
	case 14:
		k2 ^= ((uint64_t)buf8[13]) << 40;
	case 13:
		k2 ^= ((uint64_t)buf8[12]) << 32;
	case 12:
		k2 ^= ((uint64_t)buf8[11]) << 24;
	case 11:
		k2 ^= ((uint64_t)buf8[10]) << 16;
	case 10:
		k2 ^= ((uint64_t)buf8[9]) << 8;
	case  9:
		k2 ^= ((uint64_t)buf8[8]) << 0;
	case  8:
		k1 ^= ((uint64_t)buf8[7]) << 56;
	case  7:
		k1 ^= ((uint64_t)buf8[6]) << 48;
	case  6:
		k1 ^= ((uint64_t)buf8[5]) << 40;
	case  5:
		k1 ^= ((uint64_t)buf8[4]) << 32;
	case  4:
		k1 ^= ((uint64_t)buf8[3]) << 24;
	case  3:
		k1 ^= ((uint64_t)buf8[2]) << 16;
	case  2:
		k1 ^= ((uint64_t)buf8[1]) << 8;
	case  1:
		k1 ^= ((uint64_t)buf8[0]) << 0;
	}

	k2 *= c2;
	k2  = rotl64(k2,33);
	k2 *= c1;
	h2 ^= k2;

	k1 *= c1;
	k1  = rotl64(k1,31);
	k1 *= c2;
	h1 ^= k1;

	//And the last steps

	h1 ^= len; h2 ^= len;

	h1 += h2;
	h2 += h1;

	h1 = fmix64(h1);
	h2 = fmix64(h2);

	h1 += h2;
	h2 += h1;

	//Save the result
	out[0] = h1;
	out[1] = h2;

	return;
}





