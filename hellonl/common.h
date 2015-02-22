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
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef int64_t s64;
typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;

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
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif


// --- MEMORY ---
void * zalloc(size_t size);
size_t strlcpy(char *dest, const char *src, size_t siz);
static inline void * realloc_array(void *ptr, size_t nmemb, size_t size) {
	if (size && nmemb > (~(size_t)0) / size) return NULL;
	return realloc(ptr, nmemb * size);
}




#include "eloop.h"

#endif