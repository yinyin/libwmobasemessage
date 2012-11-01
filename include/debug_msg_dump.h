#ifndef __YKX_DEBUG_MSG_DUMP__
#define __YKX_DEBUG_MSG_DUMP__ 1


#include <string.h>
#include <stdio.h>



static void __print_errno_string(const char * msg, const char * subject_filename, const char * src_file, int src_line, int errno_val)
{
#if __DUMP_DEBUG_MSG
	char errno_strbuf[1024];

	memset(errno_strbuf, 0, 1024);
	strerror_r(errno_val, errno_strbuf, 1023);

	if('\0' == errno_strbuf[0])
	{ strncpy(errno_strbuf, "(ERRNO not found)", 1023); }

	if(NULL == subject_filename)
	{ fprintf(stderr, "%s: %s [@%s:%d]\n", msg, errno_strbuf, src_file, src_line); }
	else
	{ fprintf(stderr, "%s (file name: %s): %s @[%s:%d]\n", msg, subject_filename, errno_strbuf, src_file, src_line); }

#endif	/* __DUMP_DEBUG_MSG */

	return;
}



#endif	/* #ifndef __YKX_DEBUG_MSG_DUMP__ */


/*
vim: ts=4 sw=4 ai nowrap
*/
