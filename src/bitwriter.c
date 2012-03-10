#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include <endian.h>
#include <errno.h>

#include <stdio.h>
#include <string.h>

#include "bitwriter.h"


#define __DUMP_DEBUG_MSG 1

#define RESULT_FILE_PERMISSION ( S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH )


static int __bitwriter_flush_BitWriter(BitWriter * bufobj, int *errno_valptr);

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


/** 重置 BitWriter 資料結構
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 * */
void bitwriter_reset_instance_data(BitWriter *bufobj)
{
	memset(bufobj, 0, sizeof(BitWriter));

	bufobj->fd = -1;
	bufobj->bit_buffer_remain = 64;

	bufobj->current_flush_ptr = (uint64_t *)(bufobj->flush_block);
}


/** 開啟檔案準備寫入
 *
 * Argument:
 *    const char *filename - 要寫入的檔案路徑
 *    BitWriter *bufobj - BitWriter 物件
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *    0  - 正常結束
 *    -1 - 開啟檔案時發生錯誤
 * */
int bitwriter_open(const char *filename, BitWriter *bufobj, int *errno_valptr)
{
	int fd;

	bitwriter_reset_instance_data(bufobj);	/* init data structure */

	/* {{{ open file descriptor */
	if( -1 == (fd = open(filename, O_CREAT|O_WRONLY, RESULT_FILE_PERMISSION) ) )
	{
		int errno_val;

		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot open file for write", filename, __FILE__, __LINE__, errno_val);	/* dump error message */

		return -1;
	}
	/* }}} open file descriptor */
	bufobj->fd = fd;

	return 0;
}

/** 寫出所有 buffer 內容並關閉檔案
 * 注意: 若在寫出 buffer 時發生錯誤，不會回傳異常值，但是 errno 值會被設定
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *    0  - 正常結束
 *    -1 - 關閉檔案時發生錯誤
 * */
int bitwriter_close(BitWriter *bufobj, int *errno_valptr)
{
	int written_block_size;

	if(-1 == __bitwriter_flush_BitWriter(bufobj, errno_valptr))
	{ return -1; }

	if(64 != bufobj->bit_buffer_remain)	/* flush remaining bits into buffer */
	{
		int byte_count;
		uint8_t *flush_bound;
		byte_count = 8 - (bufobj->bit_buffer_remain / 8);

		*(bufobj->current_flush_ptr) = htobe64(bufobj->bit_buffer);
		flush_bound = (uint8_t *)(bufobj->current_flush_ptr) + byte_count;

		written_block_size = flush_bound - bufobj->flush_block;
	}
	else
	{
		written_block_size = (void *)(bufobj->current_flush_ptr) - (void *)(bufobj->flush_block);
	}

	/* {{{ flush remaining bits into file */
	if(written_block_size > 0)
	{
		int ret;

		if( -1 == (ret = write(bufobj->fd, bufobj->flush_block, written_block_size)) )
		{
			int errno_val;

			*errno_valptr = (errno_val = errno);
			__print_errno_string("ERR: cannot flush buffer to file", NULL, __FILE__, __LINE__, errno_val);	/* dump error message */
		}
	}

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, "INFO: flushing to file. (size=%d) @[%s:%d]\n", written_block_size, __FILE__, __LINE__);
	#endif
	/* }}} flush remaining bits into file */

	/* {{{ close file descriptor */
	if( -1 == close(bufobj->fd) )
	{
		int errno_val;

		*errno_valptr = (errno_val = errno);
		__print_errno_string("ERR: cannot close file", NULL, __FILE__, __LINE__, errno_val);	/* dump error message */

		return -1;
	}
	/* }}} open file descriptor */

	bitwriter_reset_instance_data(bufobj);	/* clear data structure */

	return 0;
}


/** (internal function)
 * 如果緩衝區已滿，將緩衝區內的資料寫出
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *    寫出的 byte 數
 * */
static int __bitwriter_flush_BitWriter(BitWriter * bufobj, int *errno_valptr)
{
	if(0 != bufobj->bit_buffer_remain)
	{ return 0; }

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, "INFO: flushing to memory block. @[%s:%d]\n", __FILE__, __LINE__);
	#endif

	*(bufobj->current_flush_ptr) = htobe64(bufobj->bit_buffer);

	bufobj->current_flush_ptr++;;
	bufobj->bit_buffer_remain = 64;
	bufobj->bit_buffer = 0L;

	if( ((void *)(bufobj->current_flush_ptr)) >= ((void *)(bufobj->flush_block + BITWRITER_FLUSH_SIZE)) )
	{
		int ret;

		if( -1 == (ret = write(bufobj->fd, bufobj->flush_block, BITWRITER_FLUSH_SIZE)) )
		{
			int errno_val;

			*errno_valptr = (errno_val = errno);
			__print_errno_string("ERR: cannot flush buffer to file", NULL, __FILE__, __LINE__, errno_val);	/* dump error message */
		}

		bufobj->current_flush_ptr = (uint64_t *)(bufobj->flush_block);

		#if __DUMP_DEBUG_MSG
			fprintf(stderr, "INFO: flushed to file. (bytes=%d) @[%s:%d]\n", ret, __FILE__, __LINE__);
		#endif

		return ret;
	}

	return 0;
}

/** 寫入指定位元的整數值
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    uint64_t writting_value - 要寫入的值
 *    int bits_desire - 要寫入的位元數
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *    寫出的 bit 數，當異常時 (一般而言是在緩衝區滿寫入檔案時發生) 傳回 -1
 * */
int bitwriter_write_integer_bits(BitWriter * bufobj, uint64_t writting_value, int bits_desire, int *errno_valptr)
{
	int bits_written;

	if(-1 == __bitwriter_flush_BitWriter(bufobj, errno_valptr))
	{ return -1; }

	if( (64 == bufobj->bit_buffer_remain) && (64 == bits_desire) )	/* handling boundary case */
	{
		bufobj->bit_buffer = writting_value;
		bufobj->bit_buffer_remain = 0;

		bits_written = 64;
	}
	else
	{
		int bits_countdown;
		uint64_t writting_bits;

		bits_written = 0;

		bits_countdown = bits_desire;
		writting_bits = ( writting_value << (64 - bits_desire) );

		while(bits_countdown > 0) {
			int buffer_remain;
			int bits_this_round;
			uint64_t going_to_write;
			uint64_t going_to_remain;

			buffer_remain = bufobj->bit_buffer_remain;
			bits_this_round = (bits_countdown < buffer_remain) ? bits_countdown : buffer_remain;

			going_to_write = (writting_bits >> (64 - buffer_remain)) & (((0x1L << bits_this_round) - 1L) << (buffer_remain - bits_this_round));
			going_to_remain = writting_bits << bits_this_round;

			bufobj->bit_buffer = bufobj->bit_buffer | going_to_write;
			bufobj->bit_buffer_remain -= bits_this_round;

			writting_bits = going_to_remain;

			bits_written += bits_this_round;
			bits_countdown -= bits_this_round;

			if(-1 == __bitwriter_flush_BitWriter(bufobj, errno_valptr))
			{ return -1; }
		}
	}

	return bits_written;
}

/** 以指定的字元寬度寫出指定數量的字元
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    char * target_chars - 指向要寫出的字串的指標
 *    int char_count - 要寫出的字元數
 *    int char_bits - 寫出時每字元的位元數
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *    寫出的 bit 數，當異常時 (一般而言是在緩衝區滿寫入檔案時發生) 傳回 -1
 * */
int bitwriter_write_string_bits(BitWriter * bufobj, char * target_chars, int char_count, int char_bits, int *errno_valptr)
{
	int char_write_count;
	char * char_storage_ptr;
	
	if( (char_count < 0) || (char_bits < 0) || (char_bits > 64) )
	{ return -2; }
	
	char_write_count = char_count;
	char_storage_ptr = target_chars;
	
	if(8 == char_bits)
	{
		while(8 <= char_write_count) {
			uint64_t aux_bits;
			int ret;
			
			aux_bits = *((uint64_t *)(char_storage_ptr));
			aux_bits = be64toh(aux_bits);
			
			if(0 > (ret = bitwriter_write_integer_bits(bufobj, aux_bits, 64, errno_valptr)))
			{ return -3; }
			
			char_storage_ptr += 8;
			char_write_count -= 8;
		}
	}
	
	while(char_write_count > 0) {
		uint64_t aux_bits;
		int ret;
		
		aux_bits = ((uint64_t)(*char_storage_ptr)) & 0xFFL;
		
		if(0 > (ret = bitwriter_write_integer_bits(bufobj, aux_bits, char_bits, errno_valptr)))
		{ return -4; }
		
		char_storage_ptr++;
		char_write_count--;
	}
	
	return char_count;
}



/*
vim: ts=4 sw=4 ai nowrap
*/
