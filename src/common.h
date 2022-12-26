/*
 * common.h <z64.me>
 *
 * common functions
 *
 */

#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED 1

#include <stdlib.h>
#include <stdint.h>

void *loadfile(const char *fn, size_t *sz);
int savefile(const char *fn, const void *dat, const size_t sz);
void die(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void Log(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
void *Memdup(const void *src, size_t len);
char *Strdup(const char *str);

uint32_t BEr32(const void *p);
uint16_t BEr16(const void *p);
uint8_t BEr8(const void *p);

void BEw16(void *dst, uint16_t v);

int min_int(const int a, const int b);
int max_int(const int a, const int b);
int min4_int(const int a, const int b, const int c, const int d);
int max4_int(const int a, const int b, const int c, const int d);

#define U32_BYTES(X) (((X) >> 24) & 0xff), (((X) >> 16) & 0xff), (((X) >> 8) & 0xff), ((X) & 0xff)
#define U24_BYTES(X) (((X) >> 16) & 0xff), (((X) >> 8) & 0xff), ((X) & 0xff)

#endif /* COMMON_H_INCLUDED */
