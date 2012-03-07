#include <stdio.h>
#include <string.h>

#include <stdint.h>

#include "filemmapio.h"
#include "wmomessage.h"


int main(int argc, char ** argv)
{
	int fd;
	uint32_t filesize;
	int errnoval;
	void * p;

	if( NULL == (p = open_file_mmap(argv[1], &fd, &filesize, &errnoval)) )
	{
		fprintf(stderr, "ERR: cannot retrive mmap\n");
		return 0;
	}

	printf("MMAP opened (pointer: %p, size: %u)\n", p, filesize);

	{
		char * q;
		char * b;
		char * r;
		int heading_len;
		uint32_t heading_type;
		WMOmessage_AbbrHeading parsed_info;

		char captured_heading[32];

		int msg_count;

		q = (char *)p;
		b = (char *)(p + filesize);
		r = NULL;
		msg_count = 0;

		do {

			r = search_abbr_heading(q, b, &heading_len, &heading_type, &parsed_info);

			if(NULL == r)
			{
				printf("Not found.\n");
				break;
			}

			memset(captured_heading, 0, 32);
			strncpy(captured_heading, r, heading_len);
			printf(">>>%s<<< type=0x%08X, len=%d\n", captured_heading, heading_type, heading_len);

			printf("> TTAA = %c %c %c %c\n", parsed_info.T1, parsed_info.T2, parsed_info.A1, parsed_info.A2);
			printf("> ii = [%s] (%d)\n", parsed_info.ii, parsed_info.ii_value);
			printf("> CCCC = [%s]\n", parsed_info.CCCC);
			printf("> YYGGgg = [%s] (d=%d, h=%d, m=%d)\n", parsed_info.YYGGgg, parsed_info.day, parsed_info.hour, parsed_info.minute);
			printf("> BBB = [%s]\n", parsed_info.BBB);

			q = r + heading_len;
			msg_count++;
		} while(NULL != r);

		printf("--> count: %d\n", msg_count);
	}

	close_file_mmap(p, &fd, &filesize);

	return 0;
}
