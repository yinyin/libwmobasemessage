#ifndef __YKX_BIT_WRITER_H__
#define __YKX_BIT_WRITER_H__ 1

/** Defines bit writing data structures and functions */

#include <stdint.h>


#define BITWRITER_FLUSH_SIZE 4096


typedef struct _T_BitWriter {
	uint8_t flush_block[BITWRITER_FLUSH_SIZE];

	int fd;

	int bit_buffer_remain;
	uint64_t bit_buffer;

	uint64_t *current_flush_ptr;

} BitWriter;



void bitwriter_reset_instance_data(BitWriter *bufobj);

int bitwriter_open(const char *filename, BitWriter *bufobj, int *errno_valptr);

int bitwriter_close(BitWriter *bufobj, int *errno_valptr);


int bitwriter_write_integer_bits(BitWriter * bufobj, uint64_t writting_value, int bits_desire, int *errno_valptr);

int bitwriter_write_string_bits(BitWriter * bufobj, char * target_chars, int char_count, int char_bits, int *errno_valptr);

int bitwriter_pad_to_byte(BitWriter * bufobj, int *errno_valptr);



#endif	/* #ifndef __YKX_BIT_WRITER_H__ */


/*
vim: ts=4 sw=4 ai nowrap
*/
