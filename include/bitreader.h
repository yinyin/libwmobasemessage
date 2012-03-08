#ifndef __YKX_BIT_READER_H__
#define __YKX_BIT_READER_H__ 1

/** Defines bit reading functions and data structure */

#include <stdint.h>


typedef struct _T_BitReader {
	void * blob_start_ptr;
	void * blob_bound_ptr;

	void * blob_current_ptr;

	void * blob_regionstart_ptr;
	void * blob_regionbound_ptr;

	int bit_buffer_remain;
	uint64_t bit_buffer;
} BitReader;



void bitreader_reset_instance_data(BitReader *inst);

void bitreader_init(BitReader *inst, void * p_blob_start, int blob_size);


int bitreader_set_current_location(BitReader *inst, void * current, int clean_buffer);

int bitreader_rewind(BitReader *inst);


void bitreader_buffer_clear(BitReader *inst);

int bitreader_buffer_truncate_to_byte(BitReader *inst);

int bitreader_buffer_giveback(BitReader *inst, int clean_buffer);


int bitreader_read_integer_bits(BitReader *inst, int bits_wanted, uint64_t *bits_storage);

int bitreader_read_string_bits(BitReader *inst, char * result_chars, int char_count, int char_bits);


void bitreader_region_set_start(BitReader *inst);

void bitreader_region_set_size(BitReader *inst, int region_size);

void bitreader_region_clear(BitReader *inst);



#endif	/* #ifndef __YKX_BIT_READER_H__ */


/*
vim: ts=4 sw=4 ai nowrap
*/
