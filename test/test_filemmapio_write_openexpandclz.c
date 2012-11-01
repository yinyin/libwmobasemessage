#include <stdio.h>
#include <string.h>


#include "filemmapio.h"

int main(int argc, char ** argv)
{
	int fd;
	uint32_t orig_filesize;
	uint32_t expd_filesize;
	int errnoval;
	char * p;

	uint32_t target_filesize;

	uint32_t expd_filesize_0;

	if( NULL == (p = open_file_write_mmap(argv[1], &fd, &orig_filesize, &expd_filesize, &errnoval)) )
	{
		fprintf(stderr, "ERR: cannot retrive mmap\n");
		return 0;
	}

	printf("MMAP opened (pointer: %p, orig_size: %u, expd_size: %u)\n", p, orig_filesize, expd_filesize);

	expd_filesize_0 = expd_filesize;
	target_filesize = expd_filesize + 89;

	{
		int i;
		for(i = 0; i < expd_filesize; i++) {
			p[i] = (127 != (i % 128)) ? 'F' : '\n';
		}
	}

	if( NULL == (p = expand_file_write_mmap(p, &fd, &expd_filesize, target_filesize, &errnoval)) )
	{
		fprintf(stderr, "ERR: cannot retrive mmap\n");
		return 0;
	}

	printf("MMAP expanded (pointer: %p, orig_size: %u, expd_size: %u)\n", p, orig_filesize, expd_filesize);

	{
		int i;
		for(i = expd_filesize_0; i < (target_filesize-1); i++) {
			p[i] = '.';
		}
		for(i = target_filesize-1; i < expd_filesize; i++) {
			p[i] = '\n';
		}
	}

	close_file_write_mmap(p, &fd, &orig_filesize, &expd_filesize, target_filesize);

	return 0;
}
