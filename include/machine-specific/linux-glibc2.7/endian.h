#ifndef	__GLIBC_20080515_ENDIAN_H__
#define	__GLIBC_20080515_ENDIAN_H__ 1

/** compatibility header for endian.h
 * This is a simple compatibility shim to have complete endian convert
 * macros on pre-20080515 version of GLIBC.
 * It is licensed under GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 * */

#ifdef htobe16
	#warning "This header file (endian.h) is for Linux system with pre-20080515 (pre-2.8) version of GLIBC.\n"
#endif	/* htobe16 */


/* definition for byte orders */
#define __LITTLE_ENDIAN 0x01234567
#define __BIG_ENDIAN    0x76543210

/* get machine dependent __BYTE_ORDER definition from system header */
#define _ENDIAN_H 1
#include <bits/endian.h>
#undef _ENDIAN_H


#include <byteswap.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
	#define htobe16(x) bswap_16 (x)
	#define htole16(x) (x)
	#define be16toh(x) bswap_16 (x)
	#define le16toh(x) (x)

	#define htobe32(x) bswap_32 (x)
	#define htole32(x) (x)
	#define be32toh(x) bswap_32 (x)
	#define le32toh(x) (x)

	#define htobe64(x) bswap_64 (x)
	#define htole64(x) (x)
	#define be64toh(x) bswap_64 (x)
	#define le64toh(x) (x)
#else
	#define htobe16(x) (x)
	#define htole16(x) bswap_16 (x)
	#define be16toh(x) (x)
	#define le16toh(x) bswap_16 (x)

	#define htobe32(x) (x)
	#define htole32(x) bswap_32 (x)
	#define be32toh(x) (x)
	#define le32toh(x) bswap_32 (x)

	#define htobe64(x) (x)
	#define htole64(x) bswap_64 (x)
	#define be64toh(x) (x)
	#define le64toh(x) bswap_64 (x)
#endif	/* #if __BYTE_ORDER == __LITTLE_ENDIAN */


#endif	/* __GLIBC_20080515_ENDIAN_H__ */
