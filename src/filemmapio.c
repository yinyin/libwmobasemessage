#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include <errno.h>

#include <stdio.h>
#include <string.h>


#include "filemmapio.h"


#define __DUMP_DEBUG_MSG 0


/** 最大檔案大小，超過此值將無法打開: 128 MiB = 134217728, 64 MiB = 67108864, 8 MiB = 8388608, 1 MiB = 1048576, 512 KiB = 524288 */
#define FILE_SIZE_LIMIT 67108864



static void __print_errno_string(const char * msg, const char * subject_filename, const char * src_file, int src_line, int errno_val)
{
#if __DUMP_DEBUG_MSG
	char errno_strbuf[1024];

	memset(errno_strbuf, 0, 1024);
	strerror_r(errno_val, errno_strbuf, 1023);

	if('\0' == errno_strbuf[0])
	{ strncpy(errno_strbuf, "(ERRNO not found)", 1023); }

	if(NULL == subject_filename)
	{ fprintf(stderr, "%s: %s [@%s:%d]\n", msg, errno_strbuf, src_file, src_line); }
	else
	{ fprintf(stderr, "%s (file name: %s): %s @[%s:%d]\n", msg, subject_filename, errno_strbuf, src_file, src_line); }

#endif	/* __DUMP_DEBUG_MSG */

	return;
}


/** open file for MMAP I/O
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

			close(fd);

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

		close(fd);

		return NULL;
	}

	if( MAP_FAILED == (result_ptr = mmap(NULL, (size_t)(filesize), PROT_READ, MAP_PRIVATE, fd, 0)) )
	{
		int errno_val;

		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot perform file mapping", filename, __FILE__, __LINE__, errno_val);	/* dump error message */

		close(fd);

		return NULL;
	}
	/* }}} create mmap */

	return result_ptr;
}


/** close MMAP I/O file
 *
 * Argument:
 * 	void * mmap_ptr - 指向 MMAP I/O 位址的指標
 * 	int * fd_ptr - 指向存放 file descriptor 的變數的指標
 * 	uint32_t * filesize_ptr - 指向存放檔案大小的變數的指標
 * */
void close_file_mmap(void * mmap_ptr, int * fd_ptr, uint32_t * filesize_ptr)
{
	munmap(mmap_ptr, (size_t)(*filesize_ptr));
	close(*fd_ptr);

	*fd_ptr = 0;
	*filesize_ptr = 0;
}



/*
vim: ts=4 sw=4 ai nowrap
*/
