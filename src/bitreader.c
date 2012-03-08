#include <stdio.h>
#include <string.h>

#include <endian.h>


#include "bitreader.h"


#define __DUMP_DEBUG_MSG 1


/** 重置 BitReader 資料結構
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 * */
void bitreader_reset_instance_data(BitReader *inst)
{
	memset(inst, 0, sizeof(BitReader));

	inst->blob_start_ptr = NULL;
	inst->blob_bound_ptr = NULL;

	inst->blob_current_ptr = NULL;

	inst->blob_regionstart_ptr = NULL;
	inst->blob_regionbound_ptr = NULL;

	inst->bit_buffer_remain = 0;
	inst->bit_buffer = 0L;
}

/** 初始化 BitReader 資料結構
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 *    void * p_blob_start - 資料塊起點指標
 *    int blob_size - 資料塊大小
 * */
void bitreader_init(BitReader *inst, void * p_blob_start, int blob_size)
{
	inst->blob_start_ptr = p_blob_start;
	inst->blob_bound_ptr = (p_blob_start + blob_size);
	inst->blob_current_ptr = p_blob_start;

	inst->blob_regionstart_ptr = inst->blob_start_ptr;
	inst->blob_regionbound_ptr = inst->blob_bound_ptr;
}


/** 設定目前讀取指標位置
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 *    void * current - 要設定成目前指標位置的指標
 *    int clean_buffer - 是否要一併清除 buffer 內容 - 0: 不清空, 1: 清空
 *
 * Return:
 *    0  - 正常結束
 *    -1 - 給定的指標超出範圍
 * */
int bitreader_set_current_location(BitReader *inst, void * current, int clean_buffer)
{
	if( (current < inst->blob_start_ptr) || (current >= inst->blob_bound_ptr) )
	{ return -1; }

	inst->blob_current_ptr = current;

	if(0 != clean_buffer)
	{ bitreader_buffer_clear(inst); }

	return 0;
}


/** 清除且捨棄目前放在 buffer 中的 bits
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 * */
void bitreader_buffer_clear(BitReader *inst)
{
	inst->bit_buffer_remain = 0;
	return;
}

/** 將 buffer 中的部份 bit 捨棄直到位元組的邊界
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 * */
int bitreader_buffer_truncate_to_byte(BitReader *inst)
{
	int bit_to_drop;

	bit_to_drop = (inst->bit_buffer_remain % 8);

	if(0 != bit_to_drop)
	{
		inst->bit_buffer = inst->bit_buffer << bit_to_drop;
		inst->bit_buffer_remain -= bit_to_drop;
	}

	return bit_to_drop;
}

/** 將 buffer 中的位元組放回資料流，未滿 1 byte 的位元會被捨棄
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 *
 * Return:
 *    放回資料流的 byte 數
 * */
int bitreader_buffer_giveback(BitReader *inst)
{
	if(inst->bit_buffer_remain >= 8)
	{
		int bytecount;

		bytecount = (inst->bit_buffer_remain / 8);
		inst->blob_current_ptr -= bytecount;

		if(inst->blob_current_ptr < inst->blob_start_ptr)
		{ inst->blob_current_ptr = inst->blob_start_ptr; }

		inst->bit_buffer_remain = 0;

		#if __DUMP_DEBUG_MSG
			fprintf(stderr, "INFO: release bits from bitbuffer, bytes=%d @[%s:%d]\n", bytecount, __FILE__, __LINE__);
		#endif

		return bytecount;
	}

	return 0;
}



/** (internal function)
 * 當 buffer 為空的時候，從資料流中讀取並填入 buffer 中
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 *
 * Return:
 *    目前在 buffer 中的 bit 數量
 * */
static int __bitreader_buffer_refill(BitReader *inst)
{
	uint64_t aux;
	int bytes_to_read;

	/* check if necessary to fill buffer */
	if(0 != inst->bit_buffer_remain)
	{ return inst->bit_buffer_remain; }

	/* read to 8 bytes boundary */
	if( 0 == (bytes_to_read = 8 -
				#if __LP64__ || __LP64
					(int)((uint64_t)(inst->blob_current_ptr) & 7L)
				#else
					(int)((uint32_t)(inst->blob_current_ptr) & 7)
				#endif
				) )
	{ bytes_to_read = 8; }

	/* make sure only read bytes inside region */
	{
		int r;

		if( (r = (int)(inst->blob_regionbound_ptr - inst->blob_current_ptr)) < bytes_to_read )
		{ bytes_to_read = r; }
	}

	if(8 == bytes_to_read)
	{	/* get 64 bits at once */
		aux = *((uint64_t *)(inst->blob_current_ptr));
		aux = be64toh(aux);
	}
	else
	{	/* get byte by byte */
		int i;
		uint8_t *p;

		p = (uint8_t *)(inst->blob_current_ptr);
		aux = 0L;

		for(i = 0; i < bytes_to_read; i++) {
			aux = aux << 8;
			aux = aux | (((uint64_t)(p[i])) & 0x00000000000000FFL);
		}

		aux = aux << (64 - (bytes_to_read*8));
	}

	inst->bit_buffer = aux;
	inst->blob_current_ptr += bytes_to_read;
	inst->bit_buffer_remain = (bytes_to_read * 8);

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, "INFO: refill bits buffer, buffer=0x%016llX, remain=%d @[%s:%d]\n", (long long unsigned int)(aux), inst->bit_buffer_remain, __FILE__, __LINE__);
	#endif

	return inst->bit_buffer_remain;
}

/** (internal function)
 * 讀取指定的位元數並視為 uint64_t 傳回
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 *    int bits_wanted - 要讀取的位元數
 *    uint64_t *bits_storage - 指向存放所讀取位元的變數的指標
 *
 * Return:
 *    成功讀取的位元數，失敗時傳回負值
 *    -1 - 資料流已經關閉
 *    -2 - 指定的位元數超過可讀取能力
 * */
static int __bitreader_buffer_read_uint64(BitReader *inst, int bits_wanted, uint64_t *bits_storage)
{
	int written_bits;
	uint64_t result_bits;

	if(NULL == inst->blob_start_ptr)
	{ return -1; }

	if( (bits_wanted < 0) || (bits_wanted > 64) )
	{ return -2; }

	written_bits = 0;
	result_bits = 0L;

	while(bits_wanted > 0) {
		int bits_to_fetch;

		if(0 == __bitreader_buffer_refill(inst))
		{ break; }

		bits_to_fetch = (bits_wanted < inst->bit_buffer_remain) ? bits_wanted : inst->bit_buffer_remain;

		if(64 == bits_to_fetch)
		{
			result_bits = inst->bit_buffer;
		}
		else
		{
			uint64_t fetched_bits;
			uint64_t remained_bits;
			uint64_t bitmask;

			fetched_bits = inst->bit_buffer >> (64 - bits_to_fetch);
			remained_bits = inst->bit_buffer << bits_to_fetch;
			bitmask = (1L << bits_to_fetch) - 1L;

			result_bits = (result_bits << bits_to_fetch) | (fetched_bits & bitmask);

			inst->bit_buffer = remained_bits;
		}

		inst->bit_buffer_remain -= bits_to_fetch;
		written_bits += bits_to_fetch;
		bits_wanted -= bits_to_fetch;
	}

	*bits_storage = result_bits;

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, "INFO: read bits, bits=0x%016llX, written=%d, buffer=0x%016llX, remain=%d @[%s:%d]\n", (long long unsigned int)(result_bits), written_bits, (long long unsigned int)(inst->bit_buffer), inst->bit_buffer_remain, __FILE__, __LINE__);
	#endif

	return written_bits;
}


/** 讀取指定的位元數並視為 uint64_t 傳回
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 *    int bits_wanted - 要讀取的位元數
 *    uint64_t *bits_storage - 指向存放所讀取位元的變數的指標
 *
 * Return:
 *    成功讀取的位元數，失敗時傳回負值
 *    -1 - 資料流已經關閉
 *    -2 - 指定的位元數超過可讀取能力
 * */
int bitreader_read_integer_bits(BitReader *inst, int bits_wanted, uint64_t *bits_storage)
{
	return __bitreader_buffer_read_uint64(inst, bits_wanted, bits_storage);
}


/** 讀取指定位元寬度的字元數傳回
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 *    char * result_chars - 指向存放所讀取字串的變數的指標
 *    int char_count - 字元數目
 *    int char_bits - 一個字元的位元數
 *
 * Return:
 *    成功讀取的位元數，失敗時傳回負值
 *    -1 - 資料流已經關閉
 *    -2 - 指定的字元數或是單一字元位元數超過可讀取能力
 *    -3, -4 - 讀取位元組時發生錯誤
 * */
int bitreader_read_string_bits(BitReader *inst, char * result_chars, int char_count, int char_bits)
{
	int char_wanted;
	char * char_storage_ptr;

	if(NULL == inst->blob_start_ptr)
	{ return -1; }

	if( (char_count < 0) || (char_bits < 0) || (char_bits > 64) )
	{ return -2; }

	char_wanted = char_count;
	char_storage_ptr = result_chars;

	/* {{{ loading bits as characters */
	if(8 == char_bits)
	{
		while(char_wanted >= 8) {
			uint64_t result_bits;
			int ret;

			if(0 > (ret = __bitreader_buffer_read_uint64(inst, 64, &result_bits)))
			{ return -3; }

			*((uint64_t *)(char_storage_ptr)) = htobe64(result_bits);
			char_wanted -= 8;
			char_storage_ptr += 8;
		}
	}

	while(char_wanted > 0) {
		uint64_t result_bits;
		int ret;

		if(0 > (ret = __bitreader_buffer_read_uint64(inst, char_bits, &result_bits)))
		{ return -4; }

		*char_storage_ptr = (char)(result_bits & 0xFFL);
		char_storage_ptr++;
		char_wanted--;
	}
	/* }}} loading bits as characters */

	*char_storage_ptr = '\0';

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, "INFO: return string: char_count=%d, char_bits=%d, (%s) @[%s:%d]\n", char_count, char_bits, result_chars, __FILE__, __LINE__);
	#endif

	return char_count;
}


/** 設定讀取區域限制起點
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 * */
void bitreader_region_set_start(BitReader *inst)
{
	inst->blob_regionstart_ptr = inst->blob_current_ptr;
	return;
}

/** 設定讀取區域限制的大小，限制的範圍由起點起算至指定的大小之內
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 *    int region_size - 指定的大小 (byte 數), 當為 -1 時等同於延伸區域限制到資料塊的限制
 * */
void bitreader_region_set_size(BitReader *inst, int region_size)
{
	if(region_size > 0)
	{
		void * p;

		p = inst->blob_regionstart_ptr + region_size;	/* compute new region pointer */
		inst->blob_regionbound_ptr = (p > inst->blob_bound_ptr) ? inst->blob_bound_ptr : p;	/* use global bound as region bound if computed bound too large */
	}
	else
	{
		inst->blob_regionbound_ptr = inst->blob_bound_ptr;
	}

	return;
}

/** 清除讀取區域限制起點
 *
 * Argument:
 *    BitReader *inst - BitReader 物件
 * */
void bitreader_region_clear(BitReader *inst)
{
	inst->blob_regionstart_ptr = inst->blob_current_ptr;
	inst->blob_regionbound_ptr = inst->blob_bound_ptr;
}



/*
vim: ts=4 sw=4 ai nowrap
*/
