/*
 * common.c <z64.me>
 *
 * common functions
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"

/* minimal file loader
 * returns 0 on failure
 * returns pointer to loaded file on success
 */
void *loadfile(const char *fn, size_t *sz)
{
	FILE *fp;
	void *dat;
	size_t sz_;
	
	if (!sz)
		sz = &sz_;
	
	/* rudimentary error checking returns 0 on any error */
	if (
		!fn
		|| !sz
		|| !(fp = fopen(fn, "rb"))
		|| fseek(fp, 0, SEEK_END)
		|| !(*sz = ftell(fp))
		|| fseek(fp, 0, SEEK_SET)
		|| !(dat = malloc(*sz))
		|| fread(dat, 1, *sz, fp) != *sz
		|| fclose(fp)
	)
		return 0;
	
	return dat;
}

/* minimal file writer
 * returns 0 on failure
 * returns non-zero on success
 */
int savefile(const char *fn, const void *dat, const size_t sz)
{
	FILE *fp;
	
	/* rudimentary error checking returns 0 on any error */
	if (
		!fn
		|| !sz
		|| !dat
		|| !(fp = fopen(fn, "wb"))
		|| fwrite(dat, 1, sz, fp) != sz
		|| fclose(fp)
	)
		return 0;
	
	return 1;
}

void die(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
		vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
	
	exit(EXIT_FAILURE);
}

void Log(const char *fmt, ...)
{
	va_list args;
	
	va_start(args, fmt);
		vfprintf(stderr, fmt, args);
	va_end(args);
	fprintf(stderr, "\n");
}

uint32_t BEr32(const void *p)
{
	const uint8_t *b = p;
	
	if (!p)
		return 0;
	
	return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

uint16_t BEr16(const void *p)
{
	const uint8_t *b = p;
	
	if (!p)
		return 0;
	
	return (b[0] << 8) | b[1];
}

uint8_t BEr8(const void *p)
{
	const uint8_t *b = p;
	
	if (!p)
		return 0;
	
	return b[0];
}

void BEw16(void *dst, uint16_t v)
{
	uint8_t *b = dst;
	
	if (!dst)
		return;
	
	b[0] = v >> 8;
	b[1] = v;
}

void *Memdup(const void *src, size_t len)
{
	void *dst = malloc(len);
	
	if (!dst)
		return 0;
	
	return memcpy(dst, src, len);
}
