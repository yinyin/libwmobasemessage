#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>

#include <endian.h>
#include <errno.h>

#include <stdio.h>

#include "filemmapio.h"
#include "bitwriter.h"


#define __DUMP_DEBUG_MSG 1
#define __DUMP_DEBUG_MSG_DETAIL 0
#include "debug_msg_dump.h"



static int __bitwriter_expand_mmap(BitWriter * bufobj, int size_to_write, int *errno_valptr);

static int __bitwriter_check_write_exceed_region(BitWriter * bufobj, int bytes_desire);

static int __bitwriter_fill_buffer_to_alignment(BitWriter * bufobj);



/** 重置 BitWriter 資料結構
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 * */
void bitwriter_reset_instance_data(BitWriter *bufobj)
{
	memset(bufobj, 0, sizeof(BitWriter));

	bufobj->blob_fd = -1;

	bufobj->orig_filesize = 0;
	bufobj->expd_filesize = 0;

	bufobj->blob_start_ptr = NULL;
	bufobj->blob_bound_ptr = NULL;

	bufobj->blob_current_ptr = NULL;
	bufobj->blob_farthest_ptr = NULL;

	bufobj->blob_regionstart_ptr = NULL;
	bufobj->blob_regionbound_ptr = NULL;

	bufobj->buffer_write_mode = BITWRITER_BUFFER_WRITE_MODIFY;

	bufobj->bit_buffer_remain = 64;
}


/** 開啟檔案準備寫入
 *
 * Argument:
 *    const char *filename - 要寫入的檔案路徑
 *    BitWriter *bufobj - BitWriter 物件
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *     0 - 正常結束
 *    -1 - 開啟檔案時發生錯誤
 * */
int bitwriter_open(const char *filename, BitWriter *bufobj, int *errno_valptr)
{
	bitwriter_reset_instance_data(bufobj);	/* init data structure */

	/* {{{ open file descriptor */
	if( NULL == (bufobj->blob_start_ptr = open_file_write_mmap(filename, &bufobj->blob_fd, &bufobj->orig_filesize, &bufobj->expd_filesize, errno_valptr)) )
	{
		__print_errno_string("ERR: cannot open file for write", filename, __FILE__, __LINE__, *errno_valptr);	/* dump error message */
		return -1;
	}
	/* }}} open file descriptor */

	bufobj->blob_bound_ptr = bufobj->blob_start_ptr + (int)(bufobj->expd_filesize);

	bufobj->blob_current_ptr = bufobj->blob_start_ptr;
	bufobj->blob_farthest_ptr = bufobj->blob_current_ptr;

	bufobj->blob_regionstart_ptr = bufobj->blob_start_ptr;
	bufobj->blob_regionbound_ptr = NULL;

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
 *     0 - 正常結束
 *    -1 - 關閉檔案時發生錯誤
 * */
int bitwriter_close(BitWriter *bufobj, int *errno_valptr)
{
	int retcode;

	retcode = 0;

	/* {{{ flush remaining bits into mmap */
	{
		int flushed_bit_count;

		if( 0 > (flushed_bit_count = bitwriter_buffer_flush(bufobj, errno_valptr)) )
		{
			__print_errno_string("ERR: failed on flush buffer", NULL, __FILE__, __LINE__, *errno_valptr);	/* dump error message */
			retcode -= 65536;
		}
		
		#if __DUMP_DEBUG_MSG
			fprintf(stderr, "INFO: flushing to memory. (bits=%d) @[%s:%d]\n", flushed_bit_count, __FILE__, __LINE__);
		#endif
	}
	/* }}} flush remaining bits into mmap */

	/* {{{ close file descriptor */
	{
		int ret;
		uint32_t actual_filesize;
		
		actual_filesize = (uint32_t)(bufobj->blob_farthest_ptr - bufobj->blob_start_ptr);
		
		if( 0 > (ret = close_file_write_mmap(bufobj->blob_start_ptr, &bufobj->blob_fd, &bufobj->orig_filesize, &bufobj->expd_filesize, actual_filesize, errno_valptr)) )
		{
			__print_errno_string("ERR: failed on closing file", NULL, __FILE__, __LINE__, *errno_valptr);	/* dump error message */
			retcode += ret;
		}

		#if __DUMP_DEBUG_MSG
			fprintf(stderr, "INFO: closed file. (ret=%d) @[%s:%d]\n", ret, __FILE__, __LINE__);
		#endif
	}
	/* }}} close file descriptor */

	bitwriter_reset_instance_data(bufobj);	/* clear data structure */

	return retcode;
}



/** 取得目前指標位置與資料流開頭的偏移量
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *
 * Return:
 *    偏移量
 * */
int64_t bitwriter_get_current_offset(BitWriter *bufobj)
{
	int64_t offset;

	offset = (int64_t)(bufobj->blob_current_ptr - bufobj->blob_start_ptr);

	return offset;
}

/** 將目前指標位置移到指定的偏移量處
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    int64_t offset - 偏移量
 *    int flush_buffer - 是否要一併寫出 buffer 內容 - 0: 不寫出, 1: 寫出
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *     0 - 正常結束
 *    -1 - 寫出緩衝時發生錯誤
 *    -2 - 偏移量為負數
 * */
int bitwriter_set_current_offset(BitWriter *bufobj, int64_t offset, int flush_buffer, int *errno_valptr)
{
	void *proposed_pointer;

	if(offset < 0)
	{ return -2; }

	proposed_pointer = bufobj->blob_start_ptr + (size_t)(offset);

	if(-1 == bitwriter_buffer_flush(bufobj, errno_valptr))
	{ return -1; }

	bufobj->blob_current_ptr = proposed_pointer;

	return 0;
}



/** (internal function)
 * 當要寫入的資料大於剩餘的 mmap 空間時，擴充 mmap 空間並調整 BitWriter 結構體中的指標
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    int size_to_write - 要寫入的大小
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *     0 - 正常
 *    -1 - 在擴充 mmap 空間時發生異常
 * */
static int __bitwriter_expand_mmap(BitWriter * bufobj, int size_to_write, int *errno_valptr)
{
	if( (bufobj->blob_current_ptr + size_to_write) <= bufobj->blob_bound_ptr )
	{ return 0; }

	/* {{{ perform mmap expand and relocate pointers in structure */
	{
		uint32_t target_size;
		void *new_map;

		target_size = (uint32_t)((size_t)(bufobj->blob_current_ptr - bufobj->blob_start_ptr) + (size_t)(size_to_write));

		if( NULL == (new_map = expand_file_write_mmap(bufobj->blob_start_ptr, &bufobj->blob_fd, &bufobj->expd_filesize, target_size, errno_valptr)) )
		{
			__print_errno_string("ERR: cannot expand mmap", NULL, __FILE__, __LINE__, *errno_valptr);	/* dump error message */
			return -1;
		}
		
		if(new_map != bufobj->blob_start_ptr)
		{
			bufobj->blob_current_ptr = new_map + (size_t)(bufobj->blob_current_ptr - bufobj->blob_start_ptr);
			bufobj->blob_farthest_ptr = new_map + (size_t)(bufobj->blob_farthest_ptr - bufobj->blob_start_ptr);
			
			bufobj->blob_regionstart_ptr = new_map + (size_t)(bufobj->blob_regionstart_ptr - bufobj->blob_start_ptr);
			if(NULL != bufobj->blob_regionbound_ptr)
			{ bufobj->blob_regionbound_ptr = new_map + (size_t)(bufobj->blob_regionbound_ptr - bufobj->blob_start_ptr); }
			
			bufobj->blob_start_ptr = new_map;
		}
		
		bufobj->blob_bound_ptr = bufobj->blob_start_ptr + (size_t)(bufobj->expd_filesize);
	}
	/* }}} perform mmap expand and relocate pointers in structure */
	
	return 0;
}


/** (internal function)
 * 檢查欲寫出的位元組數是否超過邊界
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    int bytes_desire - 要寫入的位元組數
 *
 * Return:
 *    可以寫出的位元組數
 * */
static int __bitwriter_check_write_exceed_region(BitWriter * bufobj, int bytes_desire)
{
	if(NULL != bufobj->blob_regionbound_ptr)
	{
		if( (bufobj->blob_current_ptr + bytes_desire) > bufobj->blob_regionbound_ptr )
		{ bytes_desire = (int)(bufobj->blob_regionbound_ptr - bufobj->blob_current_ptr); }
	}

	return bytes_desire;
}


/** 將目前指標前的數據填入位元緩衝區內，使目前指標與位元組字組邊界對齊
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *
 * Return:
 *    >=0 填入的 byte 數
 *    -1  當指標過度接近記憶體首端，使得無法完整讀取一個字組
 *    -2  當指標過度接近記憶體尾端，使得無法完整讀取一個字組
 * */
static int __bitwriter_fill_buffer_to_alignment(BitWriter * bufobj)
{
	int fraction;
	uint64_t r_content;
	int remain_bits;

	if(64 != bufobj->bit_buffer_remain)
	{ return 0; }

	#if __DUMP_DEBUG_MSG_DETAIL
		fprintf(stderr, "INFO: remained bitbuffer (buffer_remain=%d).\n", bufobj->bit_buffer_remain);
	#endif

	#if __LP64__ || __LP64
	fraction = (int)((uint64_t)(bufobj->blob_current_ptr) & 7LL);
	#else
	fraction = (int)((uint32_t)(bufobj->blob_current_ptr) & 7L);
	#endif

	if(0 == fraction)
	{ return 0; }

	if( bufobj->blob_regionstart_ptr > (bufobj->blob_current_ptr - fraction) )
	{ return -1; }

	if( (bufobj->blob_current_ptr - fraction + 8) > bufobj->blob_bound_ptr )
	{ return -2; }

	bufobj->blob_current_ptr -= fraction;

	r_content = *(uint64_t *)(bufobj->blob_current_ptr);
	remain_bits = 64 - (fraction * 8);

	bufobj->bit_buffer = be64toh(r_content) & ~((1LL << remain_bits) - 1LL);
	bufobj->bit_buffer_remain = remain_bits;

	return fraction;
}


/** 將位元緩衝區內的資料寫出
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *    >=0 寫出的 bit 數
 *    -1  當 I/O 異常時
 *    -32 當在 region 外起始寫入作業
 * */
int bitwriter_buffer_flush(BitWriter * bufobj, int *errno_valptr)
{
	if(64 == bufobj->bit_buffer_remain)
	{ return 0; }

	if(bufobj->blob_current_ptr < bufobj->blob_regionstart_ptr)
	{ return -32; }

	/* {{{ put existed bits */
	if( (0 != bufobj->bit_buffer_remain) && (BITWRITER_BUFFER_WRITE_MODIFY == bufobj->buffer_write_mode) )
	{
		int max_readable;
		uint64_t existed_value;

		if( 8 > (max_readable = (int)(bufobj->blob_bound_ptr - bufobj->blob_current_ptr)) )
		{
			char *p;
			char *q;
			int i;

			existed_value = 0LL;
			p = (char *)(&existed_value);
			q = (char *)(bufobj->blob_current_ptr);
			for(i = 0; i < max_readable; i++) {
				p[i] = q[i];
			}
		}
		else
		{ existed_value = *((uint64_t *)(bufobj->blob_current_ptr)); }

		bufobj->bit_buffer = bufobj->bit_buffer | ( be64toh(existed_value) & ((1LL << bufobj->bit_buffer_remain) - 1LL) );
	}
	/* }}} put existed bits */

	/* {{{ flush bitbuffer */
	{
		int byte_count;
		uint64_t flush_buffer;

		int ret;

		byte_count = 8 - (bufobj->bit_buffer_remain / 8);
		flush_buffer = htobe64(bufobj->bit_buffer);

		if( (8 == byte_count) && (
			#if __LP64__ || __LP64
				(0LL == ((uint64_t)(bufobj->blob_current_ptr) & 7LL))
			#else
				(0 == ((uint32_t)(bufobj->blob_current_ptr) & 7))
			#endif
			) && (8 == __bitwriter_check_write_exceed_region(bufobj, 8)) )
		{
			if(0 > __bitwriter_expand_mmap(bufobj, 8, errno_valptr))
			{
				#if __DUMP_DEBUG_MSG
					fprintf(stderr, "ERR: failed on expanding mmap. @[%s:%d]\n", __FILE__, __LINE__);
				#endif
				return -1;
			}

			*(uint64_t *)(bufobj->blob_current_ptr) = flush_buffer;
			bufobj->blob_current_ptr += 8;

			/* update farthest pointer */
			if(bufobj->blob_current_ptr > bufobj->blob_farthest_ptr)
			{ bufobj->blob_farthest_ptr = bufobj->blob_current_ptr; }

			ret = 64;
		}
		else
		{
			if( 0 > (ret = bitwriter_write_bytes_on_byte_boundary(bufobj, (char *)(&flush_buffer), byte_count, 0, errno_valptr)) )
			{
				#if __DUMP_DEBUG_MSG
					fprintf(stderr, "ERR: failed on writing buffer content. (byte_count=%d, ret=%d) @[%s:%d]\n", byte_count, ret, __FILE__, __LINE__);
				#endif
				return ret;
			}
		}

		bufobj->bit_buffer_remain = 64;
		bufobj->bit_buffer = 0LL;

		return ret;
	}
	/* }}} flush bitbuffer */

	return -1;
}



/** 寫入指定的位元組
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    char * writting_value - 要寫入的值
 *    int bytes_desire - 要寫入的位元組數
 *    int flush_bit_buffer - 是否在寫出前清空 bit buffer 並填補到位元邊際
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *    >=0 寫出的 bit 數
 *    -1  當 I/O 異常時 (一般而言是在緩衝區滿寫入檔案時發生)
 *    -32 當在 region 外起始寫入作業
 * */
int bitwriter_write_bytes_on_byte_boundary(BitWriter * bufobj, char *writting_value, int bytes_desire, int flush_bit_buffer, int *errno_valptr)
{
	if(0 != flush_bit_buffer)
	{
		int ret;
		if( 0 > (ret = bitwriter_buffer_flush(bufobj, errno_valptr)) )
		{ return ret; }
	}

	/* {{{ check if exceed boundary */
	if(NULL != bufobj->blob_regionbound_ptr)
	{
		if( (bufobj->blob_current_ptr + bytes_desire) > bufobj->blob_regionbound_ptr )
		{ bytes_desire = (int)(bufobj->blob_regionbound_ptr - bufobj->blob_current_ptr); }
	}

	if(bufobj->blob_current_ptr < bufobj->blob_regionstart_ptr)
	{ return -32; }
	/* }}} check if exceed boundary */

	if(bytes_desire <= 0)
	{ return 0; }

	if(0 > __bitwriter_expand_mmap(bufobj, bytes_desire, errno_valptr))
	{ return -1; }

	memcpy(bufobj->blob_current_ptr, writting_value, bytes_desire);

	bufobj->blob_current_ptr += bytes_desire;

	/* update farthest pointer */
	if(bufobj->blob_current_ptr > bufobj->blob_farthest_ptr)
	{ bufobj->blob_farthest_ptr = bufobj->blob_current_ptr; }

	return (bytes_desire * 8);
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

		/* {{{ refill bitbuffer when possible for alignment */
		{
			int refilled_bytes;
			refilled_bytes = __bitwriter_fill_buffer_to_alignment(bufobj);
			#if __DUMP_DEBUG_MSG
				fprintf(stderr, "INFO: refilled bit buffer. (byte_count=%d) @[%s:%d]\n", refilled_bytes, __FILE__, __LINE__);
			#endif
		}
		/* }}} refill bitbuffer when possible for alignment */

		while(bits_countdown > 0) {
			int buffer_remain;
			int bits_this_round;
			uint64_t going_to_write;
			uint64_t going_to_remain;

			buffer_remain = bufobj->bit_buffer_remain;
			bits_this_round = (bits_countdown < buffer_remain) ? bits_countdown : buffer_remain;

			if(0 == buffer_remain)
			{
				int ret;
				if( 0 > (ret = bitwriter_buffer_flush(bufobj, errno_valptr)) )
				{
					__print_errno_string("ERR: failed on flush bitbuffer", NULL, __FILE__, __LINE__, *errno_valptr);	/* dump error message */
					return ret;
				}
			}

			going_to_write = (writting_bits >> (64 - buffer_remain)) & (((0x1LL << bits_this_round) - 1LL) << (buffer_remain - bits_this_round));
			going_to_remain = writting_bits << bits_this_round;

			bufobj->bit_buffer = bufobj->bit_buffer | going_to_write;
			bufobj->bit_buffer_remain -= bits_this_round;

			writting_bits = going_to_remain;

			bits_written += bits_this_round;
			bits_countdown -= bits_this_round;
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
		
		aux_bits = ((uint64_t)(*char_storage_ptr)) & 0xFFLL;
		
		if(0 > (ret = bitwriter_write_integer_bits(bufobj, aux_bits, char_bits, errno_valptr)))
		{ return ((-32 == ret) ? -32 : -4); }
		
		char_storage_ptr++;
		char_write_count--;
	}
	
	return char_count;
}


/** 填補到位元組邊界
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    int *errno_valptr - 當錯誤時存放 errno 的變數的指標
 *
 * Return:
 *    寫出的 bit 數，當異常時 (一般而言是在緩衝區滿寫入檔案時發生) 傳回 -1
 * */
int bitwriter_pad_to_byte(BitWriter * bufobj, int *errno_valptr)
{
	int bits_to_pad;

	bits_to_pad = (bufobj->bit_buffer_remain) % 8;
	
	if(0 == bits_to_pad)	/* already at byte boundary */
	{ return 0; }
	
	return bitwriter_write_integer_bits(bufobj, 0LL, bits_to_pad, errno_valptr);
}



/** 設定寫出區域限制起點
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    int offset - 與目前位置的偏移量
 * */
void bitwriter_region_set_start(BitWriter *bufobj, int offset)
{
	void * p;

	p = bufobj->blob_current_ptr + offset;

	bufobj->blob_regionstart_ptr = ( (p >= bufobj->blob_start_ptr) && (p < bufobj->blob_bound_ptr) ) ? p : bufobj->blob_current_ptr;

	return;
}

/** 設定寫出區域限制的大小，限制的範圍由起點起算至指定的大小之內
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 *    int region_size - 指定的大小 (byte 數), 當為 -1 時等同於延伸區域限制到資料塊的限制
 * */
void bitwriter_region_set_size(BitWriter *bufobj, int region_size)
{
	if(region_size > 0)
	{
		void * p;

		p = bufobj->blob_regionstart_ptr + region_size;	/* compute new region pointer */
		bufobj->blob_regionbound_ptr = (p > bufobj->blob_bound_ptr) ? bufobj->blob_bound_ptr : p;	/* use global bound as region bound if computed bound too large */

		#if __DUMP_DEBUG_MSG
			fprintf(stderr, "INFO: set region size (start=%p, bound=%p) @[%s:%d]\n", bufobj->blob_regionstart_ptr, bufobj->blob_regionbound_ptr, __FILE__, __LINE__);
		#endif
	}
	else
	{
		bufobj->blob_regionbound_ptr = NULL;
	}

	return;
}

/** 清除寫出區域限制起點
 *
 * Argument:
 *    BitWriter *bufobj - BitWriter 物件
 * */
void bitwriter_region_clear(BitWriter *bufobj)
{
	bufobj->blob_regionstart_ptr = bufobj->blob_current_ptr;
	bufobj->blob_regionbound_ptr = NULL;
}



/*
vim: ts=4 sw=4 ai nowrap
*/
