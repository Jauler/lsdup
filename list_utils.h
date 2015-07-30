/*
 * Utilities for MPMCQ and LF_MAP lists
 *
 * Author: Rytis Karpuška
 *         rytis.karpuska@gmail.com
 *
 */

#ifndef __LIST_ITERATION_H
#define __LIST_ITERATION_H

//NOTE: These macros only valid in multithreaded environment if list structure
//does not change!!!

//Data acquisition
#define L_DATA(list)			((list)->data)
#define L_KEY(list)				((list)->n_key)

//Movement
#define L_NEXT(list)			((list)->next.ptr.ptr)

//Iteration
#define L_FOREACH(item, list)	for(item = list; item != NULL; item = L_NEXT(item))

#endif

