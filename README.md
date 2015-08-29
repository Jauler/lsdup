lsdup
======================================

A utility to list duplicate files in a given
folder on a unix or posix system


Dependencies and requirements
======================================

These packages are required in order to succesfully compile utility:
 * gcc (tested with gcc 4.8.4 and 5.2.0)
 * glibc
 * pthread
 * make

Also GCC needs to support these features on target platform:
 * __atomic_compare_exchange_8 for 32 bit CPUs
 * __atomic_compare_exchange_16 for 64 bit CPUs
 * __atomic_compare_exchange
 * __int128 for 64 bit machines
 * __atomic_add_fetch
 * __atomic_sub_fetch


Build
======================================

Assuming, that all dependencies are satified

To build a binary:
$ make

To cleanup:
$ make clean


