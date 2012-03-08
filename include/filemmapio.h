#ifndef __YKX_FILE_MMAP_IO__
#define __YKX_FILE_MMAP_IO__ 1

/** Defines utility functions for MMAP I/O on files */

#include <stdint.h>


/** 注意:
 *
 * 在 .c 原始碼檔案中定義有最大檔案大小上限，需注意設定為可吻合實務狀況需要的數值
 * #define FILE_SIZE_LIMIT 8192
 * */


void * open_file_mmap(const char * filename, int * fd_ptr, uint32_t * filesize_ptr, int * errno_valptr);

void close_file_mmap(void * mmap_ptr, int * fd_ptr, uint32_t * filesize_ptr);


#endif	/* #ifndef __YKX_FILE_MMAP_IO__ */


/*
vim: ts=4 sw=4 ai nowrap
*/
