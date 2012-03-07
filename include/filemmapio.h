#ifndef __YKX_FILE_MMAP_IO__
#define __YKX_FILE_MMAP_IO__ 1

/** Defines utility functions for MMAP I/O on files */

#include <stdint.h>


void * open_file_mmap(const char * filename, int * fd_ptr, uint32_t * filesize_ptr, int * errno_valptr);

void close_file_mmap(void * mmap_ptr, int * fd_ptr, uint32_t * filesize_ptr);


#endif	/* #ifndef __YKX_FILE_MMAP_IO__ */


/*
vim: ts=4 sw=4 ai nowrap
*/
