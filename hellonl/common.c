#include "common.h"

void * zalloc(size_t size)
{
	void *n = malloc(size);
	if (n) memset(n, 0, size);
	return n;
}

int get_realtime(struct realtime *t) {
	struct timespec ts;
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		t->sec = ts.tv_sec;
		t->usec = ts.tv_nsec / 1000;
		return 0;
	} else {
		return -1;
	}
}

size_t strlcpy(char *dest, const char *src, size_t siz)
{
	const char *s = src;
	size_t left = siz;

	if (left) {
		/* Copy string up to the maximum size of the dest buffer */
		while (--left != 0) {
			if ((*dest++ = *s++) == '\0')
				break;
		}
	}

	if (left == 0) {
		/* Not enough room for the string; force NUL-termination */
		if (siz != 0)
			*dest = '\0';
		while (*s++)
			; /* determine total src string length */
	}

	return s - src - 1;
}



void fhexdump(struct _IO_FILE *fd, const char *title, const u8 *buf, size_t len) {
	size_t i;
	if (!fd) return;
	fprintf(fd, "%s - hexdump(len=%lu):", title, (unsigned long)len);
	if (buf == NULL) {
		fprintf(fd, " [NULL]");
	} else {
		for (i = 0; i < len; i++)
			fprintf(fd, " %02x", buf[i]);
	}
	fprintf(fd, "\n");
}

void * __hide_aliasing_typecast(void *foo)
{
	return foo;
}