#ifndef __YKX_BIT_WRITER_H__
#define __YKX_BIT_WRITER_H__ 1

/** Defines bit writing data structures and functions */

#include <stdint.h>



typedef struct _T_BitWriter {
	int blob_fd;

	int __x_padding_1;

	uint32_t orig_filesize;
	uint32_t expd_filesize;

	void * blob_start_ptr;
	void * blob_bound_ptr;

	void * blob_current_ptr;
	void * blob_farthest_ptr;

	void * blob_regionstart_ptr;
	void * blob_regionbound_ptr;

	uint32_t buffer_write_mode;

	int bit_buffer_remain;
	uint64_t bit_buffer;
} BitWriter;



#define BITWRITER_BUFFER_WRITE_MODIFY 0
#define BITWRITER_BUFFER_WRITE_OVERWRITE 1



void bitwriter_reset_instance_data(BitWriter *bufobj);

int bitwriter_open(const char *filename, BitWriter *bufobj, int *errno_valptr);

int bitwriter_close(BitWriter *bufobj, int *errno_valptr);


int64_t bitwriter_get_current_offset(BitWriter *bufobj);

int bitwriter_set_current_offset(BitWriter *bufobj, int64_t offset, int flush_buffer, int *errno_valptr);


int bitwriter_buffer_flush(BitWriter * bufobj, int *errno_valptr);

int bitwriter_write_bytes_on_byte_boundary(BitWriter * bufobj, char *writting_value, int bytes_desire, int flush_bit_buffer, int *errno_valptr);

int bitwriter_write_integer_bits(BitWriter * bufobj, uint64_t writting_value, int bits_desire, int *errno_valptr);

int bitwriter_write_string_bits(BitWriter * bufobj, char * target_chars, int char_count, int char_bits, int *errno_valptr);

int bitwriter_pad_to_byte(BitWriter * bufobj, int *errno_valptr);


void bitwriter_region_set_start(BitWriter *bufobj, int offset);

void bitwriter_region_set_size(BitWriter *bufobj, int region_size);

void bitwriter_region_clear(BitWriter *bufobj);



#endif	/* #ifndef __YKX_BIT_WRITER_H__ */


/*
vim: ts=4 sw=4 ai nowrap
*/
