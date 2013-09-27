#include <unistd.h>
#include <stdint.h>

#include <stdio.h>
#include <string.h>

#include "wmomessage.h"


#define __DUMP_DEBUG_MSG 0


/* {{{ search abbr heading ------------------------------------------ */

#define _SEARCHABBRHEDG_MIN_LEN (4+1+4+1+6+3)

#define _EVALABBRHEDG_INPUT_NCHR 0
#define _EVALABBRHEDG_INPUT_RCHR 1
#define _EVALABBRHEDG_INPUT_SPAC 2
#define _EVALABBRHEDG_INPUT_NUMB 3
#define _EVALABBRHEDG_INPUT_ALPH 4

#define _EVALABBRHEDG_STATE_END 26

/* {{{ status transition table
--------
import re

IN_MAP = {'NCHR': 0, 'RCHR': 1, 'SPAC': 2, 'NUMB': 3, 'ALPH': 4}

x = """
 0 ALPH 1
 1 ALPH 2
 2 ALPH 3
 3 ALPH 4
 4 ALPH 4
 4 NUMB 5
 4 SPAC 7
 5 NUMB 6
 6 SPAC 7
 7 ALPH 8
 7 SPAC 7
 8 ALPH 9
 9 ALPH 10
10 ALPH 11
11 SPAC 12
11 ALPH 4
11 NUMB 5
12 SPAC 12
12 NUMB 13
12 ALPH 8
13 NUMB 14
14 NUMB 15
15 NUMB 16
16 NUMB 17
17 NUMB 18
18 SPAC 19
18 RCHR 24
19 SPAC 19
19 ALPH 20
19 RCHR 24
20 ALPH 21
21 ALPH 22
22 SPAC 23
22 RCHR 24
22 ALPH 4
23 SPAC 23
23 RCHR 24
24 RCHR 25
25 NCHR 26
26 =END-STATE
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
		except Exception as e:
			print ">>> line = %r" % (l,)
			raise e

for in_v in range(1+max_input):
	print "\t{", ", ".join([ ("%2d"%xt) for xt in result[in_v] ]), "},"

--------
}}} */

static int _ABBR_HEADING_SEARCHING_TBL[5][27] = {
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 26,  0 },
	{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 24, 24,  0,  0, 24, 24, 25,  0,  0 },
	{  0,  0,  0,  0,  7,  0,  7,  7,  0,  0,  0, 12, 12,  0,  0,  0,  0,  0, 19, 19,  0,  0, 23, 23,  0,  0,  0 },
	{  0,  0,  0,  0,  5,  6,  0,  0,  0,  0,  0,  5, 13, 14, 15, 16, 17, 18,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	{  1,  2,  3,  4,  4,  0,  0,  8,  9, 10, 11,  4,  8,  0,  0,  0,  0,  0,  0, 20, 21, 22,  4,  0,  0,  0,  0 }
};

static void __fill_abbr_heading_info_structure(int next_state, char input_ch, char * detected_ttaa_start, uint32_t heading_type, WMOmessage_AbbrHeading * parsed_info_ptr)
{
	switch(next_state)
	{
	case 4:
		memset(parsed_info_ptr, 0, sizeof(WMOmessage_AbbrHeading));
		break;

	/* T1T2A1A2 */
	case 26:
		parsed_info_ptr->T1 = detected_ttaa_start[0];
		parsed_info_ptr->T2 = detected_ttaa_start[1];
		parsed_info_ptr->A1 = detected_ttaa_start[2];
		parsed_info_ptr->A2 = detected_ttaa_start[3];
		/* {{{ clean out ii if is CWB-D1 type */
		if(0 != (heading_type & WMOMSG_ABBRHEADING_TYPE_CWBD1))
		{
			parsed_info_ptr->ii[0] = '\0'; parsed_info_ptr->ii[1] = '\0';
			parsed_info_ptr->ii_value = 0;
		}
		/* }}} clean out ii if is CWB-D1 type */
		break;

	/* ii */
	case 5:
		parsed_info_ptr->ii[0] = input_ch;
		break;
	case 6:
		parsed_info_ptr->ii[1] = input_ch;
		/* {{{ compute ii_value */
		{
			char ii0, ii1;
			ii0 = parsed_info_ptr->ii[0]; ii1 = parsed_info_ptr->ii[1];
			parsed_info_ptr->ii_value = (uint8_t)( ((int)(ii0 - '0'))*10 + ((int)(ii1 - '0')) );
		}
		/* }}} compute ii_value */
		break;

	/* CCCC */
	case 8:
		parsed_info_ptr->CCCC[0] = input_ch;
		break;
	case 9:
		parsed_info_ptr->CCCC[1] = input_ch;
		break;
	case 10:
		parsed_info_ptr->CCCC[2] = input_ch;
		break;
	case 11:
		parsed_info_ptr->CCCC[3] = input_ch;
		break;

	/* YYGGgg */
	case 13:
		parsed_info_ptr->YYGGgg[0] = input_ch;
		break;
	case 14:
		parsed_info_ptr->YYGGgg[1] = input_ch;
		break;
	case 15:
		parsed_info_ptr->YYGGgg[2] = input_ch;
		break;
	case 16:
		parsed_info_ptr->YYGGgg[3] = input_ch;
		break;
	case 17:
		parsed_info_ptr->YYGGgg[4] = input_ch;
		break;
	case 18:
		parsed_info_ptr->YYGGgg[5] = input_ch;
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

	/* BBB */
	case 20:
		parsed_info_ptr->BBB[0] = input_ch;
		break;
	case 21:
		parsed_info_ptr->BBB[1] = input_ch;
		break;
	case 22:
		parsed_info_ptr->BBB[2] = input_ch;
		break;
	}

	return;
}

static int __translate_input_for_matching_abbr_heading(char input_ch)
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

static int __lookup_next_state_for_forward_match_abbr_heading(int input_val, int current_state)
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

static char * _forward_match_abbr_heading(char * p_start, char * p_bound, int * heading_len_ptr, uint32_t * heading_type_ptr, WMOmessage_AbbrHeading * parsed_info_ptr)
{
	int current_state;
	char * detected_ttaa_start;
	char * detected_cccc_start;
	uint32_t heading_type;
	char * p;
	char * b;

	current_state = 0;
	detected_ttaa_start = NULL;
	detected_cccc_start = NULL;
	heading_type = 0;
	p = p_start;
	b = p_bound;

	while(p < b) {
		char input_ch;
		int input_val;
		int next_state;

		input_ch = *p;
		input_val = __translate_input_for_matching_abbr_heading(input_ch);	/* convert input character into value */
		next_state = -1;

		/* look up next state, return NULL if -1 is returned */
		if( -1 == (next_state = __lookup_next_state_for_forward_match_abbr_heading(input_val, current_state)) )
		{ return NULL; }

		/* actions for certain state transitions */
		if( ((5 == next_state) || (7 == next_state)) && ((4 == current_state) || (11 == current_state)) )
		{	/* 偵測到 TTAA 序列，紀錄並開始計數 */
			detected_ttaa_start = p - 4;
			heading_type = ((7==next_state)&&(4==current_state)) ? WMOMSG_ABBRHEADING_TYPE_CWBD1 /* CWB D1 模式 */ : 0;
		}
		else if( (8 == next_state) && (12 == current_state) )
		{	/* 偵測到 CCCC 序列其實可能為無 ii 的 TTAA 序列，紀錄並開始計數 */
			detected_ttaa_start = detected_cccc_start;
			detected_cccc_start = p;
			heading_type = WMOMSG_ABBRHEADING_TYPE_CWBD1;	/* CWB D1 模式 */
		}
		else if( (8 == next_state) && (7 == current_state) )
		{	/* 偵測到 CCCC 序列，紀錄起點 */
			detected_cccc_start = p;
		}
		else if( (20 == next_state) && (19 == current_state) )
		{	/* 進入 BBB indicator */
			heading_type |= WMOMSG_ABBRHEADING_TYPE_INDBBB;
		}
		else if( (0 == next_state) || (-1 == input_val) )
		{ detected_ttaa_start = NULL; }

		/* parse infomation into structure when requested */
		if(NULL != parsed_info_ptr)
		{ __fill_abbr_heading_info_structure(next_state, input_ch, detected_ttaa_start, heading_type, parsed_info_ptr); }

		if(_EVALABBRHEDG_STATE_END == next_state)
		{	/* 進入結束狀態 */
			*heading_len_ptr = 1 + (int)(p - detected_ttaa_start);
			*heading_type_ptr = heading_type;
			return detected_ttaa_start;
		}


		current_state = next_state;	/* transit state */

		p++;
	}

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, ">> ### REACH END(TOP) OF STREAM [@%s:%d]\n", __FILE__, __LINE__);
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
	char * r;
	int heading_len;
	uint32_t heading_type;

	r = _forward_match_abbr_heading(start, bound, &heading_len, &heading_type, parsed_info_ptr);
	#if __DUMP_DEBUG_MSG
		fprintf(stderr, "...... result = %p [@%s:%d]\n", r, __FILE__, __LINE__);
	#endif	/* __DUMP_DEBUG_MSG */

	if(NULL != r)
	{
		if(NULL != heading_len_ptr)
		{ *heading_len_ptr = heading_len; }
		if(NULL != heading_type_ptr)
		{ *heading_type_ptr = heading_type; }

		return ((void *)(r));
	}

	return NULL;
}

/* }}} search abbr heading ------------------------------------------ */


/* {{{ search BUFR indicator ---------------------------------------- */

#define _EVALBUFRIDENTN_INPUT_BCHR 0
#define _EVALBUFRIDENTN_INPUT_UCHR 1
#define _EVALBUFRIDENTN_INPUT_FCHR 2
#define _EVALBUFRIDENTN_INPUT_RCHR 3

#define _EVALBUFRIDENTN_STATE_END 4

/* {{{ status transition table
--------
import re

IN_MAP = {'BCHR': 0, 'UCHR': 1, 'FCHR': 2, 'RCHR': 3}

x = """
 0 BCHR 1
 1 UCHR 2
 2 FCHR 3
 3 RCHR 4
 4 =END-STATE
"""

max_state = 4
max_input = 3

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
		except Exception as e:
			print ">>> line = %r" % (l,)
			raise e

for in_v in range(1+max_input):
	print "\t{", ", ".join([ ("%2d"%xt) for xt in result[in_v] ]), "},"

--------
}}} */


static int _BUFR_IDENTIFICATION_SEARCHING_TBL[4][5] = {
	{  1,  0,  0,  0,  0 },
	{  0,  2,  0,  0,  0 },
	{  0,  0,  3,  0,  0 },
	{  0,  0,  0,  4,  0 }
};

static int __translate_input_for_search_bufr_identification(char input_ch)
{
	int input_val;

	input_val = -1;

	if( 'B' == input_ch )
	{ input_val = _EVALBUFRIDENTN_INPUT_BCHR; }
	else if( 'U' == input_ch )
	{ input_val = _EVALBUFRIDENTN_INPUT_UCHR; }
	else if( 'F' == input_ch )
	{ input_val = _EVALBUFRIDENTN_INPUT_FCHR; }
	else if( 'R' == input_ch )
	{ input_val = _EVALBUFRIDENTN_INPUT_RCHR; }

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, ">> ... INPUT_CHAR=0x%02X, VAL=%d [@%s:%d]\n", input_ch, input_val, __FILE__, __LINE__);
	#endif	/* __DUMP_DEBUG_MSG */

	return input_val;
}

static int __lookup_next_state_for_search_bufr_identification(int input_val, int current_state)
{
	int next_state;

	/* {{{ look up next state */
	next_state = (-1 == input_val) ?
		#if 0
			-1:	/* disallow unknown characters, will-&-should cause state machine return NULL */
		#else
			0 :	/* allow unknown characters */
		#endif
			_BUFR_IDENTIFICATION_SEARCHING_TBL[input_val][current_state];
	/* }}} look up next state */

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, ">> CURR_STATE=%d, NEXT_STATE=%d [@%s:%d]\n", current_state, next_state, __FILE__, __LINE__);
	#endif	/* __DUMP_DEBUG_MSG */

	return next_state;
}

static char * _search_bufr_identification(char * p_start, char * p_bound)
{
	int current_state;
	char * detected_bufridentn_start;
	char * p;
	char * b;

	current_state = 0;
	detected_bufridentn_start = NULL;
	p = p_start;
	b = p_bound;

	while(p < b) {
		char input_ch;
		int input_val;
		int next_state;

		input_ch = *p;
		input_val = __translate_input_for_search_bufr_identification(input_ch);	/* convert input character into value */
		next_state = -1;

		/* look up next state, return NULL if -1 is returned */
		if( -1 == (next_state = __lookup_next_state_for_search_bufr_identification(input_val, current_state)) )
		{ return NULL; }

		/* actions for certain state transitions */
		if(1 == next_state)
		{	/* 偵測到 BUFR 字串起頭字元，紀錄並開始計數 */
			detected_bufridentn_start = p;
		}
		else if( (0 == next_state) || (-1 == input_val) )
		{ detected_bufridentn_start = NULL; }

		if(_EVALBUFRIDENTN_STATE_END == next_state)
		{	/* 進入結束狀態 */
			return detected_bufridentn_start;
		}


		current_state = next_state;	/* transit state */

		p++;
	}

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, ">> ### REACH END(TOP) OF STREAM [@%s:%d]\n", __FILE__, __LINE__);
	#endif	/* __DUMP_DEBUG_MSG */

	return NULL;
}

/** 在指定的資料流範圍中找尋 BUFR identification 字元
 * 依據 WMO 306 規範 (WMO Manual on Codes, Volume I, International Codes, Part B-Binary Codes, WMO-No.306, FM 94-IX Ext. BUFR.)
 *
 * Argument:
 * 	void * start - 起點指標
 * 	void * bound - 終點指標
 * */
void * search_bufr_identification(void * start, void * bound)
{
	char * r;

	r = _search_bufr_identification(start, bound);
	#if __DUMP_DEBUG_MSG
		fprintf(stderr, "...... result = %p [@%s:%d]\n", r, __FILE__, __LINE__);
	#endif	/* __DUMP_DEBUG_MSG */

	return (NULL != r) ? ((void *)(r)) : NULL;
}

/* }}} search BUFR indicator ---------------------------------------- */



/* {{{ search ETX --------------------------------------------------- */

#define _EVALETX_INPUT_CR 0
#define _EVALETX_INPUT_LF 1
#define _EVALETX_INPUT_ETX 2

#define _EVALETX_STATE_END 3

/* {{{ status transition table
--------
import re

IN_MAP = {'CR': 0, 'LF': 1, 'ETX': 2,}

x = """
 0 CR 1
 1 LF 2
 2 ETX 3
 3 =END-STATE
"""

max_state = 3
max_input = 2

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
		except Exception as e:
			print ">>> line = %r" % (l,)
			raise e

for in_v in range(1+max_input):
	print "\t{", ", ".join([ ("%2d"%xt) for xt in result[in_v] ]), "},"

--------
}}} */

static int _ETX_SIGNAL_SEARCHING_TBL[3][4] = {
	{  1,  0,  0,  0 },
	{  0,  2,  0,  0 },
	{  0,  0,  3,  0 }
};

static int __translate_input_for_search_ETX_signal(char input_ch)
{
	int input_val;

	input_val = -1;

	if( (char)(0x0D) == input_ch )
	{ input_val = _EVALETX_INPUT_CR; }
	else if( (char)(0x0A) == input_ch )
	{ input_val = _EVALETX_INPUT_LF; }
	else if( (char)(0x03) == input_ch )
	{ input_val = _EVALETX_INPUT_ETX; }

	#if __DUMP_DEBUG_MSG
		fprintf(stderr, ">> ... INPUT_CHAR=0x%02X, VAL=%d [@%s:%d]\n", input_ch, input_val, __FILE__, __LINE__);
	#endif	/* __DUMP_DEBUG_MSG */

	return input_val;
}

/** 在指定的資料流範圍中找尋 ETX 終止符號
 * 依據 WMO 386 規範 (WMO Manual on the Global Telecommunication System, Part-2 Operational Procedures for the GTS, 2.3.4)
 *
 * Argument:
 * 	void * start - 起點指標
 * 	void * bound - 終點指標
 * */
void * search_ETX_signal(void * start, void * bound)
{
	int current_state;
	char * p;

	current_state = 0;
	p = start;
	while(p < bound) {
		char input_ch;
		char input_val;

		input_ch = *p;
		p++;

		input_val = __translate_input_for_search_ETX_signal(input_ch);
		current_state = (-1 == input_val) ? 0 : _ETX_SIGNAL_SEARCHING_TBL[input_val][current_state];

		if(_EVALETX_STATE_END == current_state)
		{ return p; }
	}

	return NULL;
}

/* }}} search ETX --------------------------------------------------- */



/*
vim: ts=4 sw=4 ai nowrap
*/
