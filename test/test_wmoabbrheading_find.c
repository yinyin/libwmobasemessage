#include <stdio.h>
#include <string.h>

#include <stdint.h>

#include "wmomessage.h"

int main(int argc, char ** argv)
{
	char t_input[] = "2asdf abcd 123456  RsRA\r\r\neca92asdf qwer abcd 123456  RRA\r\r\nadw1awsds xbcd 823456  RRA\r\r\n2aa";

	char * r;
	int heading_len;
	uint32_t heading_type;
	WMOmessage_AbbrHeading parsed_info;

	char captured_heading[32];


	r = search_abbr_heading((void *)(t_input), (void *)(t_input+sizeof(t_input)), &heading_len, &heading_type, &parsed_info);

	if(NULL == r)
	{
		printf("Not found.\n");
		return 1;
	}

	memset(captured_heading, 0, 32);
	strncpy(captured_heading, r, heading_len);
	printf(">>>%s<<< type=0x%08X, len=%d\n", captured_heading, heading_type, heading_len);

	printf("> TTAA = %c %c %c %c\n", parsed_info.T1, parsed_info.T2, parsed_info.A1, parsed_info.A2);
	printf("> ii = [%s] (%d)\n", parsed_info.ii, parsed_info.ii_value);
	printf("> CCCC = [%s]\n", parsed_info.CCCC);
	printf("> YYGGgg = [%s] (d=%d, h=%d, m=%d)\n", parsed_info.YYGGgg, parsed_info.day, parsed_info.hour, parsed_info.minute);
	printf("> BBB = [%s]\n", parsed_info.BBB);

	return 0;
}

