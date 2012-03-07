#include <stdio.h>
#include <string.h>


#include "filemmapio.h"

int main(int argc, char ** argv)
{
	int fd;
	uint32_t filesize;
	int errnoval;
	int8_t * p;

	if( NULL == (p = open_file_mmap(argv[1], &fd, &filesize, &errnoval)) )
	{
		fprintf(stderr, "ERR: cannot retrive mmap\n");
		return 0;
	}

	printf("MMAP opened (pointer: %p, size: %u)\n", p, filesize);

	{
		int i;
		for(i = 0; i < filesize; i++) {
			printf("%4d: 0x%02d [%c]\n", i, (int)(p[i]), p[i]);
		}
	}

	close_file_mmap(p, &fd, &filesize);

	return 0;
}
