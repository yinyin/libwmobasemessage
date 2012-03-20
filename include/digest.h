#ifndef __YKX_DIGEST_H__
#define __YKX_DIGEST_H__ 1

/** define digest APIs */


#define DIGESTFMT_NONE 0
#define DIGESTFMT_HEX 1
#define DIGESTFMT_BASE64 2


void run_md5(char *result, void *p, int size, int encoding);


#endif	/* __YKX_DIGEST_H__ */


/*
vim: ts=4 sw=4 ai nowrap
*/
