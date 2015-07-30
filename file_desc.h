/*
 * File description structure for use in list of potential matches
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 *
 */


#ifndef __FILE_DESC_H
#define __FILE_DESC_H

#include <stdint.h>

struct file_desc {
	uint64_t hash[2];
	int hash_valid;

	int size;
	char filename[];
};

#endif
