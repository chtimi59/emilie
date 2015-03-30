#ifndef COMMON_H
#define COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

// --- BASIC STORAGE TYPES ---
#include <unistd.h>
#include <stdbool.h>
#include <endian.h>
#include <stdint.h>


#if defined(__linux__) || defined(__GLIBC__)
#include <endian.h>
#include <byteswap.h>
#endif /* __linux__ */

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || \
	defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/endian.h>
#define __BYTE_ORDER	_BYTE_ORDER
#define	__LITTLE_ENDIAN	_LITTLE_ENDIAN
#define	__BIG_ENDIAN	_BIG_ENDIAN
#ifdef __OpenBSD__
#define bswap_16 swap16
#define bswap_32 swap32
#define bswap_64 swap64
#else /* __OpenBSD__ */
#define bswap_16 bswap16
#define bswap_32 bswap32
#define bswap_64 bswap64
#endif /* __OpenBSD__ */
#endif /* defined(__FreeBSD__) || defined(__NetBSD__) ||
* defined(__DragonFly__) || defined(__OpenBSD__) */


#ifdef __CHECKER__
#define __force __attribute__((force))
#define __bitwise __attribute__((bitwise))
#else
#define __force
#define __bitwise
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le_to_host16(n) ((__force u16) (le16) (n))
#define host_to_le16(n) ((__force le16) (u16) (n))
#define be_to_host16(n) bswap_16((__force u16) (be16) (n))
#define host_to_be16(n) ((__force be16) bswap_16((n)))
#define le_to_host32(n) ((__force u32) (le32) (n))
#define host_to_le32(n) ((__force le32) (u32) (n))
#define be_to_host32(n) bswap_32((__force u32) (be32) (n))
#define host_to_be32(n) ((__force be32) bswap_32((n)))
#define le_to_host64(n) ((__force u64) (le64) (n))
#define host_to_le64(n) ((__force le64) (u64) (n))
#define be_to_host64(n) bswap_64((__force u64) (be64) (n))
#define host_to_be64(n) ((__force be64) bswap_64((n)))
#elif __BYTE_ORDER == __BIG_ENDIAN
#define le_to_host16(n) bswap_16(n)
#define host_to_le16(n) bswap_16(n)
#define be_to_host16(n) (n)
#define host_to_be16(n) (n)
#define le_to_host32(n) bswap_32(n)
#define host_to_le32(n) bswap_32(n)
#define be_to_host32(n) (n)
#define host_to_be32(n) (n)
#define le_to_host64(n) bswap_64(n)
#define host_to_le64(n) bswap_64(n)
#define be_to_host64(n) (n)
#define host_to_be64(n) (n)
#ifndef WORDS_BIGENDIAN
#define WORDS_BIGENDIAN
#endif
#else
#error Could not determine CPU byte order
#endif




typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

typedef u16 __bitwise be16;
typedef u16 __bitwise le16;
typedef u32 __bitwise be32;
typedef u32 __bitwise le32;
typedef u64 __bitwise be64;
typedef u64 __bitwise le64;

#define STRUCT_PACKED __attribute__ ((packed))



// --- TIMES ---
#include <time.h>
typedef long time_t;
struct realtime {
	time_t sec;
	time_t usec;
};
int get_realtime(struct realtime *t);
static inline int realtime_before(struct realtime *a, struct realtime *b) {
	return (a->sec < b->sec) || (a->sec == b->sec && a->usec < b->usec);
}

static inline void realtime_sub(struct realtime *a, struct realtime *b, struct realtime *res) {
	res->sec = a->sec - b->sec;
	res->usec = a->usec - b->usec;
	if (res->usec < 0) {
		res->sec--;
		res->usec += 1000000;
	}
}


// --- NETWORK ---
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif

#define broadcast_ether_addr (const u8 *) "\xff\xff\xff\xff\xff\xff"


// --- MEMORY ---
void * zalloc(size_t size);
size_t strlcpy(char *dest, const char *src, size_t siz);
static inline void * realloc_array(void *ptr, size_t nmemb, size_t size) {
	if (size && nmemb > (~(size_t)0) / size) return NULL;
	return realloc(ptr, nmemb * size);
}

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
void fhexdump(struct _IO_FILE *, const char *title, const u8 *buf, size_t len);


/*
* gcc 4.4 ends up generating strict-aliasing warnings about some very common
* networking socket uses that do not really result in a real problem and
* cannot be easily avoided with union-based type-punning due to struct
* definitions including another struct in system header files. To avoid having
* to fully disable strict-aliasing warnings, provide a mechanism to hide the
* typecast from aliasing for now. A cleaner solution will hopefully be found
* in the future to handle these cases.
*/
void * __hide_aliasing_typecast(void *foo);
#define aliasing_hide_typecast(a,t) (t *) __hide_aliasing_typecast((a))


#include "eloop.h"

#endif