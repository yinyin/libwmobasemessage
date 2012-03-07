#include <unistd.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include "wmomessage.h"


#define __DUMP_DEBUG_MSG 1


#define _SEARCHABBRHEDG_MIN_LEN (4+1+4+1+6+3)

#define _EVALABBRHEDG_INPUT_NCHR 0
#define _EVALABBRHEDG_INPUT_RCHR 1
#define _EVALABBRHEDG_INPUT_SPAC 2
#define _EVALABBRHEDG_INPUT_NUMB 3
#define _EVALABBRHEDG_INPUT_ALPH 4

#define _EVALABBRHEDG_STATE_END 25

/* {{{ status transition table
--------
import re

IN_MAP = {'NCHR': 0, 'RCHR': 1, 'SPAC': 2, 'NUMB': 3, 'ALPH': 4}

x = """
 0 NCHR 1
 1 RCHR 2
 2 RCHR 3
 3 SPAC 4
 3 ALPH 5
 3 NUMB 6
 4 SPAC 4
 4 ALPH 5
 4 NUMB 6
 5 ALPH 12
 6 NUMB 7
 7 NUMB 8
 8 NUMB 9
 9 NUMB 10
10 NUMB 11
11 SPAC 14
12 ALPH 13
13 SPAC 26
14 SPAC 14
14 ALPH 15
15 ALPH 16
16 ALPH 17
17 ALPH 18
18 SPAC 19
19 SPAC 19
19 NUMB 20
19 ALPH 22
20 NUMB 21
21 ALPH 22
22 ALPH 23
23 ALPH 24
24 ALPH 25
25 =END-STATE
26 SPAC 26
26 NUMB 6
"""

max_state = 26
max_input = 4

r = re.compile("\s*([0-9]+)\s+([A-Z][A-Z0-9]*)\s+([0-9]+)\s*")
result = [ [0 for i in range(1+max_state)] for j in range(1+max_input) ]

for l in x.split("\n"):
	m = r.match(l)
	if m is not None:
		try:
			xf = int(m.group(1))
			xt = int(m.group(3))
			in_symb = m.group(2)
			in_v = IN_MAP[in_symb]
			result[in_v][xf] = xt
		except e:
			print ">>> line = %r" % (l,)
			raise e

for in_v in range(1+max_input):
	print "\t{", ", ".join([ ("%2d"%xt) for xt in result[in_v] ]), "},"

--------
}}} */

static int _ABBR_HEADING_SEARCHING_TBL[5][27] = {
	{  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  0,  2,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  0,  0,  0,  4,  4,  0,  0,  0,  0,  0,  0, 14,  0, 26, 14,  0,  0,  0, 19, 19,  0,  0,  0,  0,  0,  0, 26 },
	{  0,  0,  0,  6,  6,  0,  7,  8,  9, 10, 11,  0,  0,  0,  0,  0,  0,  0,  0, 20, 21,  0,  0,  0,  0,  0,  6 },
	{  0,  0,  0,  5,  5, 12,  0,  0,  0,  0,  0,  0, 13,  0, 15, 16, 17, 18,  0, 22,  0, 22, 23, 24, 25,  0,  0 }
};

static void __fill_abbr_heading_info_structure(int next_state, char input_ch, WMOmessage_AbbrHeading * parsed_info_ptr)
{
	switch(next_state)
	{
		/* T1T2A1A2 */
		case 25:
			parsed_info_ptr->T1 = input_ch;
			break;
		case 24:
			parsed_info_ptr->T2 = input_ch;
			break;
		case 23:
			parsed_info_ptr->A1 = input_ch;
			break;
		case 22:
			parsed_info_ptr->A2 = input_ch;
			break;

		/* ii */
		case 21:
			parsed_info_ptr->ii[0] = input_ch;
			/* {{{ compute ii_value */
			{
				char ii0, ii1;
				ii0 = parsed_info_ptr->ii[0]; ii1 = parsed_info_ptr->ii[1];
				parsed_info_ptr->ii_value = (uint8_t)( ((int)(ii0 - '0'))*10 + ((int)(ii1 - '0')) );
			}
			/* }}} compute ii_value */
			break;
		case 20:
			parsed_info_ptr->ii[1] = input_ch;
			break;

		/* CCCC */
		case 18:
			parsed_info_ptr->CCCC[0] = input_ch;
			break;
		case 17:
			parsed_info_ptr->CCCC[1] = input_ch;
			break;
		case 16:
			parsed_info_ptr->CCCC[2] = input_ch;
			break;
		case 15:
			parsed_info_ptr->CCCC[3] = input_ch;
			break;

		/* YYGGgg */
		case 11:
			parsed_info_ptr->YYGGgg[0] = input_ch;
			/* {{{ compute day, hour, minute values */
			{
				char D0, D1, h0, h1, m0, m1;
				D0 = parsed_info_ptr->YYGGgg[0]; D1 = parsed_info_ptr->YYGGgg[1];
				h0 = parsed_info_ptr->YYGGgg[2]; h1 = parsed_info_ptr->YYGGgg[3];
				m0 = parsed_info_ptr->YYGGgg[4]; m1 = parsed_info_ptr->YYGGgg[5];
				parsed_info_ptr->day    = (uint8_t)( ((int)(D0 - '0'))*10 + ((int)(D1 - '0')) );
				parsed_info_ptr->hour   = (uint8_t)( ((int)(h0 - '0'))*10 + ((int)(h1 - '0')) );
				parsed_info_ptr->minute = (uint8_t)( ((int)(m0 - '0'))*10 + ((int)(m1 - '0')) );
			}
			/* }}} compute day, hour, minute values */
			break;
		case 10:
			parsed_info_ptr->YYGGgg[1] = input_ch;
			break;
		case 9:
			parsed_info_ptr->YYGGgg[2] = input_ch;
			break;
		case 8:
			parsed_info_ptr->YYGGgg[3] = input_ch;
			break;
		case 7:
			parsed_info_ptr->YYGGgg[4] = input_ch;
			break;
		case 6:
			parsed_info_ptr->YYGGgg[5] = input_ch;
			break;

		/* BBB */
		case 13:
			parsed_info_ptr->BBB[0] = input_ch;
			break;
		case 12:
			parsed_info_ptr->BBB[1] = input_ch;
			break;
		case 5:
			parsed_info_ptr->BBB[2] = input_ch;
			break;
	}

	return;
}

static int __translate_input_for_reverse_match_abbr_heading(char input_ch)
{
	int input_val;

	input_val = -1;

	if( '\n' == input_ch )
	{ input_val = _EVALABBRHEDG_INPUT_NCHR; }
	else if( '\r' == input_ch )
	{ input_val = _EVALABBRHEDG_INPUT_RCHR; }
	else if( (' ' == input_ch) || ('\t' == input_ch) )
	{ input_val = _EVALABBRHEDG_INPUT_SPAC; }
	else if( ('0' <= input_ch) && ('9' >= input_ch) )
	{ input_val = _EVALABBRHEDG_INPUT_NUMB; }
	else if( (('A' <= input_ch) && ('Z' >= input_ch)) || (('a' <= input_ch) && ('z' >= input_ch)) )
	{ input_val = _EVALABBRHEDG_INPUT_ALPH; }

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, ">> ... INPUT_CHAR=0x%02X, VAL=%d [@%s:%d]\n", input_ch, input_val, __FILE__, __LINE__);
	#endif	/* __DUMP_DEBUG_MSG */

	return input_val;
}

static int __lookup_next_state_for_reverse_match_abbr_heading(int input_val, int current_state)
{
	int next_state;

	/* {{{ look up next state */
	next_state = (-1 == input_val) ?
		#if 0
			-1:	/* disallow unknown characters, will-&-should cause state machine return NULL */
		#else
			0 :	/* allow unknown characters */
		#endif
			_ABBR_HEADING_SEARCHING_TBL[input_val][current_state];
	/* }}} look up next state */

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, ">> CURR_STATE=%d, NEXT_STATE=%d [@%s:%d]\n", current_state, next_state, __FILE__, __LINE__);
	#endif	/* __DUMP_DEBUG_MSG */

	return next_state;
}

static char * _reverse_match_abbr_heading(char * p_start, char * p_upperbound, int * heading_len_ptr, uint32_t * heading_type_ptr, WMOmessage_AbbrHeading * parsed_info_ptr)
{
	int current_state;
	int found_len;
	uint32_t heading_type;
	char * p;
	char * b;

	current_state = 0;
	found_len = -1;
	heading_type = 0;
	p = p_start;
	b = p_upperbound;

	while(b <= p) {
		char input_ch;
		int input_val;
		int next_state;

		input_ch = *p;
		input_val = __translate_input_for_reverse_eval_is_abbr_heading(input_ch);	/* convert input character into value */
		next_state = -1;

		/* look up next state, return NULL if -1 is returned */
		if( -1 == (next_state = __lookup_next_state_for_reverse_match_abbr_heading(input_val, current_state)) )
		{ return NULL; }

		/* parse infomation into structure when requested */
		if(NULL != parsed_info_ptr)
		{ __fill_abbr_heading_info_structure(next_state, input_ch, parsed_info_ptr); }

		/* actions for certain state transitions */
		if(_EVALABBRHEDG_STATE_END == next_state)
		{	/* 進入結束狀態 */
			*heading_len_ptr = found_len;
			*heading_type_ptr = heading_type;
			return p;
		}
		else if( (1 == next_state) && (0 == current_state) )
		{	/* 找到 heading 結束字元，開始計數 */
			found_len = 1;
			heading_type = 0;

			if(NULL != parsed_info_ptr)
			{ memset(parsed_info_ptr, 0, sizeof(WMOmessage_AbbrHeading)); }
		}
		else if( (5 == next_state) && ((3 == current_state) || (4 == current_state)) )
		{	/* 進入 BBB indicator */
			heading_type |= WMOMSG_ABBRHEADING_TYPE_INDBBB;
		}
		else if( (22 == next_state) && (19 == current_state) )
		{	/* 沒有 ii, 判斷為 CWB 一組電報 */
			heading_type |= WMOMSG_ABBRHEADING_TYPE_CWBD1;
		}
		else if(-1 == input_val)
		{ found_len = -1; }


		current_state = next_state;	/* transit state */

		p--;
		found_len++;
	}

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, ">> ### REACH END(TOP) OF STREAM\n");
	#endif	/* __DUMP_DEBUG_MSG */

	return NULL;
}


/** 在指定的資料流範圍中找尋 WMO abbreviated heading
 * 依據 WMO 386 規範 (Page. II -5/1)
 *
 * Argument:
 * 	void * start - 起點指標
 * 	void * bound - 終點指標
 * 	int * heading_len_ptr - 指向要儲存標頭長度的變數的指標
 * 	int * heading_type_ptr - 指向要儲存標頭種類的變數的指標
 * 	WMOmessage_AbbrHeading * parsed_info_ptr - 指向要儲存已解譯資訊的結構體的指標，若為 NULL 則表不儲存已解譯資訊
 * */
void * search_abbr_heading(void * start, void * bound, int * heading_len_ptr, uint32_t * heading_type_ptr, WMOmessage_AbbrHeading * parsed_info_ptr)
{
	char * p;
	char * b;

	p = ((char *)(bound));
	b = ((char *)(start));

	while(p >= b) {
		char * r;

		r = _reverse_match_abbr_heading(p, b, heading_len_ptr, heading_type_ptr);
		#if __DUMP_DEBUG_MSG
			fprintf(stderr, "...... result = %p\n", r);
		#endif	/* __DUMP_DEBUG_MSG */

		if(NULL != r)
		{ return ((void *)(r)); }

		p--;
	}

	return NULL;
}



/*
vim: ts=4 sw=4 ai nowrap
*/
