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

uint32_t BEr32(const void *p);
uint16_t BEr16(const void *p);
uint8_t BEr8(const void *p);

#endif /* COMMON_H_INCLUDED */
