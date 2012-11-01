#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include <errno.h>

#include <stdio.h>


#include "filemmapio.h"


#define __DUMP_DEBUG_MSG 0
#include "debug_msg_dump.h"


/** 最大檔案大小，超過此值將無法打開: 128 MiB = 134217728, 64 MiB = 67108864, 8 MiB = 8388608, 1 MiB = 1048576, 512 KiB = 524288 */
#define FILE_SIZE_LIMIT 67108864


/** 開啟要寫入的檔案時，預先放大檔案大小的基數值: 4 KiB = 12, 8 KiB = 13, 16 KiB = 14, 32 KiB = 15, 64 KiB = 16 */
#define FILE_EXPAND_INCREMENT_STEP_IN_PWR2BITSCOUNT 12

#define FILE_EXPAND_INCREMENT_STEP_VALUE (1 << FILE_EXPAND_INCREMENT_STEP_IN_PWR2BITSCOUNT)
#define FILE_EXPAND_INCREMENT_STEP_MASK (FILE_EXPAND_INCREMENT_STEP_VALUE - 1)


#define RESULT_FILE_PERMISSION ( S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH )



/** open file for MMAP I/O read
 *
 * Argument:
 * 	const char * filename - 檔案名稱
 * 	int * fd_ptr - 指向要儲存 file descriptor 的變數的指標
 * 	uint32_t * filesize_ptr - 指向要儲存檔案大小的變數的指標
 * 	int * errno_valptr - 指向要儲存 ERRNO 值的變數的指標
 *
 * Return:
 * 	所取得的 MMAP I/O 指標，當失敗時傳回 NULL 並設定 ERRNO 值
 * */
void * open_file_read_mmap(const char * filename, int * fd_ptr, uint32_t * filesize_ptr, int * errno_valptr)
{
	int fd;
	off_t filesize;
	void * result_ptr;

	/* {{{ clear out variables */
	*fd_ptr = -1;
	*filesize_ptr = 0;
	*errno_valptr = 0;	/* clear errno pointer first */
	result_ptr = NULL;
	/* }}} clear out variables */

	/* {{{ open file descriptor */
	if( -1 == (fd = open(filename, O_RDONLY) ) )
	{
		int errno_val;
		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot open file for read", filename, __FILE__, __LINE__, errno_val);	/* dump error message */
		return NULL;
	}

	*fd_ptr = fd;
	/* }}} open file descriptor */


	/* {{{ get file size */
	{
		struct stat stat_buf;

		if(-1 == fstat(fd, &stat_buf))
		{
			int errno_val;

			*errno_valptr = (errno_val = errno);
			__print_errno_string("ERR: cannot get file size", filename, __FILE__, __LINE__, errno_val);	/* dump error message */
			close(fd);	/* close file descriptor */
			return NULL;
		}

		filesize = stat_buf.st_size;
		*filesize_ptr = ( (filesize > 0) ? (uint32_t)(filesize) : 0 );
	}
	/* }}} get file size */

	/* {{{ create mmap */
	if( (filesize <= 0) || (filesize > FILE_SIZE_LIMIT) )
	{	/* file too large */
		#if __DUMP_DEBUG_MSG
			fprintf(stderr, "ERR: file too large (> %d), (filename=[%s], size=%lld) @[%s:%d]\n", FILE_SIZE_LIMIT, filename, (long long int)(filesize), __FILE__, __LINE__);
		#endif
		close(fd);	/* close file descriptor */
		return NULL;
	}

	if( MAP_FAILED == (result_ptr = mmap(NULL, (size_t)(filesize), PROT_READ, MAP_PRIVATE, fd, 0)) )
	{
		int errno_val;
		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot perform file mapping", filename, __FILE__, __LINE__, errno_val);	/* dump error message */
		close(fd);	/* close file descriptor */
		return NULL;
	}
	/* }}} create mmap */

	return result_ptr;
}


/** close MMAP I/O read file
 *
 * Argument:
 * 	void * mmap_ptr - 指向 MMAP I/O 位址的指標
 * 	int * fd_ptr - 指向存放 file descriptor 的變數的指標
 * 	uint32_t * filesize_ptr - 指向存放檔案大小的變數的指標
 * */
void close_file_read_mmap(void * mmap_ptr, int * fd_ptr, uint32_t * filesize_ptr)
{
	munmap(mmap_ptr, (size_t)(*filesize_ptr));
	close(*fd_ptr);

	*fd_ptr = 0;
	*filesize_ptr = 0;
}



/** computed expanded size
 *
 * Argument:
 *  off_t target_size - 目標/原始大小
 *
 * Return:
 *  計算後的擴增為寫檔擴充頁倍數值
 */
static off_t compute_expanded_size(off_t target_size)
{
	if((off_t)(0) >= target_size)
	{ return FILE_EXPAND_INCREMENT_STEP_VALUE; }

	if( 0 == ((off_t)(FILE_EXPAND_INCREMENT_STEP_MASK) & target_size) )
	{ return target_size; }

	return ( ((target_size >> FILE_EXPAND_INCREMENT_STEP_IN_PWR2BITSCOUNT) + (off_t)(1)) << FILE_EXPAND_INCREMENT_STEP_IN_PWR2BITSCOUNT );
}


/** open file for MMAP I/O write
 *
 * Argument:
 * 	const char * filename - 檔案名稱
 * 	int * fd_ptr - 指向要儲存 file descriptor 的變數的指標
 * 	uint32_t * origional_filesize_ptr - 指向要儲存原始檔案大小的變數的指標
 * 	uint32_t * expanded_filesize_ptr - 指向要儲存擴展檔案大小的變數的指標
 * 	int * errno_valptr - 指向要儲存 ERRNO 值的變數的指標
 *
 * Return:
 * 	所取得的 MMAP I/O 指標，當失敗時傳回 NULL 並設定 ERRNO 值
 * */
void * open_file_write_mmap(const char * filename, int * fd_ptr, uint32_t * origional_filesize_ptr, uint32_t * expanded_filesize_ptr, int * errno_valptr)
{
	int fd;
	off_t origional_filesize;
	off_t expanded_filesize;
	void * result_ptr;

	/* {{{ clear out variables */
	*fd_ptr = -1;
	*origional_filesize_ptr = 0;
	*expanded_filesize_ptr = 0;
	*errno_valptr = 0;	/* clear errno pointer first */
	result_ptr = NULL;
	/* }}} clear out variables */

	/* {{{ open file descriptor */
	if( -1 == (fd = open(filename, O_RDWR|O_CREAT, RESULT_FILE_PERMISSION)) )
	{
		int errno_val;
		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot open file for write", filename, __FILE__, __LINE__, errno_val);	/* dump error message */
		return NULL;
	}

	*fd_ptr = fd;
	/* }}} open file descriptor */


	/* {{{ get file size */
	{
		struct stat stat_buf;

		if(-1 == fstat(fd, &stat_buf))
		{
			int errno_val;
			*errno_valptr = (errno_val = errno);
			__print_errno_string("ERR: cannot get file size", filename, __FILE__, __LINE__, errno_val);	/* dump error message */
			close(fd);	/* close file descriptor */
			return NULL;
		}

		origional_filesize = stat_buf.st_size;
		*origional_filesize_ptr = ( (origional_filesize > 0) ? (uint32_t)(origional_filesize) : 0 );
	}
	/* }}} get file size */

	/* {{{ compute expanded file size */
	expanded_filesize = (
		( 0 != ((off_t)(FILE_EXPAND_INCREMENT_STEP_MASK) & origional_filesize) )
			? ( ((origional_filesize >> FILE_EXPAND_INCREMENT_STEP_IN_PWR2BITSCOUNT) + (off_t)(1)) << FILE_EXPAND_INCREMENT_STEP_IN_PWR2BITSCOUNT )
			: ( origional_filesize )
	);
	/* }}} compute expanded file size */

	/* {{{ create mmap */
	if( (origional_filesize < 0) || (origional_filesize > FILE_SIZE_LIMIT) || (expanded_filesize < 0) )
	{	/* file too large */
		#if __DUMP_DEBUG_MSG
			fprintf(stderr, "ERR: file too large (> %d), (filename=[%s], orig_size=%lld, expand_size=%lld) @[%s:%d]\n", FILE_SIZE_LIMIT, filename, (long long int)(origional_filesize), (long long int)(expanded_filesize), __FILE__, __LINE__);
		#endif
		close(fd);	/* close file descriptor */
		return NULL;
	}

	/* ... {{{ expand file size */
	if(-1 == ftruncate(fd, expanded_filesize))
	{
		int errno_val;
		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot perform f-truncate", filename, __FILE__, __LINE__, errno_val);	/* dump error message */
		close(fd);	/* close file descriptor */
		return NULL;
	}

	*expanded_filesize_ptr = expanded_filesize;
	/* ... }}} expand file size */

	if( MAP_FAILED == (result_ptr = mmap(NULL, (size_t)(expanded_filesize), PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0)) )
	{
		int errno_val;
		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot perform file mapping", filename, __FILE__, __LINE__, errno_val);	/* dump error message */
		close(fd);	/* close file descriptor */
		return NULL;
	}
	/* }}} create mmap */

	return result_ptr;
}


/** expand file for MMAP I/O write
 *
 * Argument:
 * 	void * mmap_ptr - 指向 MMAP I/O 位址的指標
 * 	const char * filename - 檔案名稱
 * 	int * fd_ptr - 指向儲存 file descriptor 的變數的指標
 * 	uint32_t * expanded_filesize_ptr - 指向要儲存擴展檔案大小的變數的指標
 *  uint32_t target_filesize - 目標的檔案大小
 * 	int * errno_valptr - 指向要儲存 ERRNO 值的變數的指標
 *
 * Return:
 * 	所取得的 MMAP I/O 指標，當失敗時傳回 NULL 並設定 ERRNO 值
 * */
void * expand_file_write_mmap(void * mmap_ptr, int * fd_ptr, uint32_t * expanded_filesize_ptr, uint32_t target_filesize, int * errno_valptr)
{
	int fd;
	off_t expanded_filesize;
	void * result_ptr;

	/* {{{ initial variables */
	fd = *fd_ptr;
	*errno_valptr = 0;	/* clear errno pointer first */
	result_ptr = NULL;
	/* }}} initial variables */

	/* {{{ compute expanded target file size */
	expanded_filesize = (
		( 0 != ((off_t)(FILE_EXPAND_INCREMENT_STEP_MASK) & target_filesize) )
			? ( ((target_filesize >> FILE_EXPAND_INCREMENT_STEP_IN_PWR2BITSCOUNT) + (off_t)(1)) << FILE_EXPAND_INCREMENT_STEP_IN_PWR2BITSCOUNT )
			: ( target_filesize )
	);
	/* }}} compute expanded target file size */

	/* {{{ releasing mmap */
	if(-1 == munmap(mmap_ptr, (size_t)(*expanded_filesize_ptr)))
	{
		int errno_val;
		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot perform file unmapping", "[EXPANDING_MMAP]", __FILE__, __LINE__, errno_val);	/* dump error message */
		return NULL;
	}
	/* }}} releasing mmap */

	/* {{{ create mmap */
	/* ... {{{ expand file size */
	if(-1 == ftruncate(fd, expanded_filesize))
	{
		int errno_val;
		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot perform f-truncate", "[EXPANDING_MMAP]", __FILE__, __LINE__, errno_val);	/* dump error message */
		return NULL;
	}

	*expanded_filesize_ptr = expanded_filesize;
	/* ... }}} expand file size */

	if( MAP_FAILED == (result_ptr = mmap(NULL, (size_t)(expanded_filesize), PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0)) )
	{
		int errno_val;
		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot perform file mapping", "[EXPANDING_MMAP]", __FILE__, __LINE__, errno_val);	/* dump error message */
		return NULL;
	}
	/* }}} create mmap */

	return result_ptr;
}


/** close MMAP I/O write file
 *
 * Argument:
 * 	void * mmap_ptr - 指向 MMAP I/O 位址的指標
 * 	int * fd_ptr - 指向存放 file descriptor 的變數的指標
 * 	uint32_t * origional_filesize_ptr - 指向要儲存原始檔案大小的變數的指標
 * 	uint32_t * expanded_filesize_ptr - 指向要儲存擴展檔案大小的變數的指標
 *  uint32_t actual_filesize - 實際檔案應有的大小
 * */
void close_file_write_mmap(void * mmap_ptr, int * fd_ptr, uint32_t * origional_filesize_ptr, uint32_t * expanded_filesize_ptr, uint32_t actual_filesize)
{
	int fd;

	fd = *fd_ptr;

	/* {{{ close file */
	munmap(mmap_ptr, (size_t)(*expanded_filesize_ptr));
	
	if(-1 == ftruncate(fd, (off_t)(actual_filesize)))
	{
		int errno_val;
		errno_val = errno;
		__print_errno_string("ERR: cannot perform f-truncate on closing", "[CLOSING_FILE]", __FILE__, __LINE__, errno_val);	/* dump error message */
	}

	close(fd);
	/* }}} close file */

	*fd_ptr = 0;
	*origional_filesize_ptr = 0;
	*expanded_filesize_ptr = 0;
}



/*
vim: ts=4 sw=4 ai nowrap
*/
