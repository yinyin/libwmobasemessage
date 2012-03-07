#ifndef __YKX_WMO_MESSAGE__
#define __YKX_WMO_MESSAGE__ 1

/** define utility functions related to WMO message/stream parsing */

#include <stdint.h>


typedef struct _T_WMOmessage_AbbrHeading {
	char T1;
	char T2;
	char A1;
	char A2;

	char ii[3];
	uint8_t ii_value;

	char CCCC[5];
	int8_t __padding_1[3];

	char YYGGgg[7];
	int8_t __padding_2;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	int8_t __padding_3;

	char BBB[4];

} WMOmessage_AbbrHeading;

/** abbreviated heading 特殊型別指示
 * 標準規格內 (http://www.nws.noaa.gov/tg/headef.html): (使用低16位元)
 * 	WMOMSG_ABBRHEADING_TYPE_INDBBB
 * 		這個 heading 帶有 indicator BBB 的資訊 (http://www.nws.noaa.gov/tg/bbb.html)
 *
 * 標準規格外: (使用高16位元)
 * 	WMOMSG_ABBRHEADING_TYPE_CWBD1
 * 		這個 heading 屬於 CWB 一組報 (無 ii 資料)
 * */
#define WMOMSG_ABBRHEADING_TYPE_INDBBB  0x00000001
#define WMOMSG_ABBRHEADING_TYPE_CWBD1   0x00010000


void * search_abbr_heading(void * start, void * bound, int * heading_len_ptr, uint32_t * heading_type_ptr, WMOmessage_AbbrHeading * parsed_info_ptr);


#endif	/* #ifndef __YKX_WMO_MESSAGE__ */


/*
vim: ts=4 sw=4 ai nowrap
*/
