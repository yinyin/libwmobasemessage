#include <stdint.h>
#include <string.h>

#include "digest.h"


typedef struct _T_MD5_context {
	uint32_t lo, hi;
	uint32_t a, b, c, d;
	unsigned char buffer[64];
	uint32_t block[16];
} MD5_context;


/* The basic MD5 functions.
 *
 * F and G are optimized compared to their RFC 1321 definitions for
 * architectures that lack an AND-NOT instruction, just like in Colin Plumb's
 * implementation.
 */
#define F(x, y, z)			((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)			((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z)			((x) ^ (y) ^ (z))
#define I(x, y, z)			((y) ^ ((x) | ~(z)))

/* The MD5 transformation for all four rounds.
 */
#define STEP(f, a, b, c, d, x, t, s) \
	(a) += f((b), (c), (d)) + (x) + (t); \
	(a) = (((a) << (s)) | (((a) & 0xffffffff) >> (32 - (s)))); \
	(a) += (b);

/* SET reads 4 input bytes in little-endian byte order and stores them
 * in a properly aligned word in host byte order.
 *
 * The check for little-endian architectures that tolerate unaligned
 * memory accesses is just an optimization.  Nothing will break if it
 * doesn't work.
 */
#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
	#define SET(n) (*(uint32_t *)&ptr[(n) * 4])
	#define GET(n) SET(n)
#else
	#define SET(n) \
		(ctx->block[(n)] = \
		(uint32_t)ptr[(n) * 4] | \
		((uint32_t)ptr[(n) * 4 + 1] << 8) | \
		((uint32_t)ptr[(n) * 4 + 2] << 16) | \
		((uint32_t)ptr[(n) * 4 + 3] << 24))
	#define GET(n) \
		(ctx->block[(n)])
#endif

/* This processes one or more 64-byte data blocks, but does NOT update
 * the bit counters.  There are no alignment requirements.
 */
static const void * body(MD5_context *ctx, const void *data, size_t size)
{
	const unsigned char *ptr;
	uint32_t a, b, c, d;
	uint32_t saved_a, saved_b, saved_c, saved_d;

	ptr = data;

	a = ctx->a;
	b = ctx->b;
	c = ctx->c;
	d = ctx->d;

	do {
		saved_a = a;
		saved_b = b;
		saved_c = c;
		saved_d = d;

/* Round 1 */
		STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
		STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
		STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
		STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
		STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
		STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
		STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
		STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
		STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
		STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
		STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
		STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
		STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
		STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
		STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
		STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)

/* Round 2 */
		STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
		STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
		STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
		STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
		STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
		STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
		STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
		STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
		STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
		STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
		STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
		STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
		STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
		STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
		STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
		STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

/* Round 3 */
		STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
		STEP(H, d, a, b, c, GET(8), 0x8771f681, 11)
		STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
		STEP(H, b, c, d, a, GET(14), 0xfde5380c, 23)
		STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
		STEP(H, d, a, b, c, GET(4), 0x4bdecfa9, 11)
		STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
		STEP(H, b, c, d, a, GET(10), 0xbebfbc70, 23)
		STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
		STEP(H, d, a, b, c, GET(0), 0xeaa127fa, 11)
		STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
		STEP(H, b, c, d, a, GET(6), 0x04881d05, 23)
		STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
		STEP(H, d, a, b, c, GET(12), 0xe6db99e5, 11)
		STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
		STEP(H, b, c, d, a, GET(2), 0xc4ac5665, 23)

/* Round 4 */
		STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
		STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
		STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
		STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
		STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
		STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
		STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
		STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
		STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
		STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
		STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
		STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
		STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
		STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
		STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
		STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

		a += saved_a;
		b += saved_b;
		c += saved_c;
		d += saved_d;

		ptr += 64;
	} while (size -= 64);

	ctx->a = a;
	ctx->b = b;
	ctx->c = c;
	ctx->d = d;

	return ptr;
}


static void md5_init(MD5_context *ctx)
{
	ctx->a = 0x67452301;
	ctx->b = 0xefcdab89;
	ctx->c = 0x98badcfe;
	ctx->d = 0x10325476;

	ctx->lo = 0;
	ctx->hi = 0;
	
	return;
}


static void md5_update(MD5_context *ctx, const void *data, size_t size)
{
	uint32_t saved_lo;
	uint32_t usedbyte;

	saved_lo = ctx->lo;
	if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo)
	{ ctx->hi++; }
	ctx->hi += size >> 29;

	usedbyte = saved_lo & 0x3f;

	if(usedbyte)
	{
		uint32_t freesize;
		
		freesize = 64 - usedbyte;

		if (size < freesize)
		{
			memcpy(&ctx->buffer[usedbyte], data, size);
			return;
		}

		memcpy(&ctx->buffer[usedbyte], data, freesize);
		data = (unsigned char *)data + freesize;
		size -= freesize;
		body(ctx, ctx->buffer, 64);
	}

	if (size >= 64)
	{
		data = body(ctx, data, size & ~(size_t)0x3f);
		size &= 0x3f;
	}

	memcpy(ctx->buffer, data, size);
	
	return;
}


static void md5_final(unsigned char *result, MD5_context *ctx)
{
	uint32_t usedbyte, freebyte;

	usedbyte = ctx->lo & 0x3f;

	ctx->buffer[usedbyte++] = 0x80;

	freebyte = 64 - usedbyte;

	if (freebyte < 8)
	{
		memset(&ctx->buffer[usedbyte], 0, freebyte);
		body(ctx, ctx->buffer, 64);
		usedbyte = 0;
		freebyte = 64;
	}

	memset(&ctx->buffer[usedbyte], 0, freebyte - 8);

	ctx->lo <<= 3;
	ctx->buffer[56] = ctx->lo;
	ctx->buffer[57] = ctx->lo >> 8;
	ctx->buffer[58] = ctx->lo >> 16;
	ctx->buffer[59] = ctx->lo >> 24;
	ctx->buffer[60] = ctx->hi;
	ctx->buffer[61] = ctx->hi >> 8;
	ctx->buffer[62] = ctx->hi >> 16;
	ctx->buffer[63] = ctx->hi >> 24;

	body(ctx, ctx->buffer, 64);

	result[0] = ctx->a;
	result[1] = ctx->a >> 8;
	result[2] = ctx->a >> 16;
	result[3] = ctx->a >> 24;
	result[4] = ctx->b;
	result[5] = ctx->b >> 8;
	result[6] = ctx->b >> 16;
	result[7] = ctx->b >> 24;
	result[8] = ctx->c;
	result[9] = ctx->c >> 8;
	result[10] = ctx->c >> 16;
	result[11] = ctx->c >> 24;
	result[12] = ctx->d;
	result[13] = ctx->d >> 8;
	result[14] = ctx->d >> 16;
	result[15] = ctx->d >> 24;

	memset(ctx, 0, sizeof(*ctx));
	
	return;
}


/** 針對指定的資料計算 MD5 簽章
 *
 * Argument:
 *      char *result - 存放結果的記憶體空間
 *      void *p - 指向要進行簽章的記憶體的指標
 *      int size - 要簽章的記憶體大小
 *      int encoding - 編碼方式 (DIGESTFMT_NONE: 不編碼 (16 bytes), DIGESTFMT_HEX: 16 進位字串 (32 bytes), DIGESTFMT_BASE64: Base64 編碼 (22 bytes)
 * */
void run_md5(char *result, void *p, int size, int encoding)
{
	MD5_context ctx;
	uint8_t r[18];
	
	memset(r, 0, 18);
	
	md5_init(&ctx);
	md5_update(&ctx, p, (size_t)(size));
	md5_final(r, &ctx);
	
	/* dump result to storage */
	if(DIGESTFMT_NONE == encoding)
	{ memcpy(result, r, 16); }
	else if(DIGESTFMT_HEX == encoding)
	{
		const char HEX_MAP[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
		int i;
		for(i = 0; i < 16; i++) {
			result[i*2+0] = HEX_MAP[ (r[i] >> 4) & 0xF ];
			result[i*2+1] = HEX_MAP[ (r[i] >> 0) & 0xF ];
		}
	}
	else if(DIGESTFMT_BASE64 == encoding)
	{
		const char B64_MAP[64] = {
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
			'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
			'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};
		int i, j;
		for(i=0, j=0; i < 16; i+=3, j+=4) {
			result[j+0] = B64_MAP[ (r[i+0] >> 2) & 0x3F ];
			result[j+1] = B64_MAP[ ((r[i+0] << 4) & 0x30) | ((r[i+1] >> 4) & 0x0F) ];
			if(j >= 20)
			{ break;	/* Base64 MD5 has 22 characters */ }
			result[j+2] = B64_MAP[ ((r[i+1] << 2) & 0x3C) | ((r[i+2] >> 6) & 0x03) ];
			result[j+3] = B64_MAP[ (r[i+2] >> 0) & 0x3F ];
		}
	}
	
	return;
}



/*
vim: ts=4 sw=4 ai nowrap
*/
