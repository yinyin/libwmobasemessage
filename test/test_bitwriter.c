#include <stdio.h>
#include <stdint.h>

#include "bitwriter.h"

int main(int argc, char ** argv)
{
	BitWriter writer;
	int errno_val;
	int bitcount;

	char CODEMAP[62];

	int i;

	if(-1 == bitwriter_open("BINWRITE.out", &writer, &errno_val))
	{
		fprintf(stderr, "ERR: cannot open file.\n");
		return 1;
	}

	for(i=0; i<26; i++) {
		CODEMAP[i+0] = (char)(i+((int)('a')));
	}
	for(i=0; i<26; i++) {
		CODEMAP[i+26] = (char)(i+((int)('A')));
	}
	for(i=0; i<10; i++) {
		CODEMAP[i+52] = (char)(i+((int)('0')));
	}

	for(i = 0; i < 18; i++) {
		uint64_t vl;

		vl = (uint64_t)(CODEMAP[ i%62 ]);

		if( -1 == (bitcount = bitwriter_write_integer_bits(&writer, vl, 8, &errno_val)) )
		{
			fprintf(stderr, "ERR: cannot write to file.\n");
			return 1;
		}
	}


	if( -1 == (bitcount = bitwriter_write_integer_bits(&writer, 9L, 6, &errno_val)) )
	{
		fprintf(stderr, "ERR: cannot write to file.\n");
		return 1;
	}


	for(i = 0; i < 256; i++) {
		uint64_t vl;

		vl = (uint64_t)(CODEMAP[ i%62 ]);

		if( -1 == (bitcount = bitwriter_write_integer_bits(&writer, vl, 8, &errno_val)) )
		{
			fprintf(stderr, "ERR: cannot write to file.\n");
			return 1;
		}
	}

#if 1
	if(-1 == bitwriter_close(&writer, &errno_val))
	{
		fprintf(stderr, "ERR: failed on closing file.\n");
	}

#endif

	return 0;
}
