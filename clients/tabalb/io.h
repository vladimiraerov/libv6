/**
 * io.h
 *
 * Dave Taht
 * 2017-03-13
 */

#ifndef IO_H
#define IO_H
#include "shared.h"

extern const char *format_address(const ubase_t a);
extern const char *format_prefix(const ubase_t a);
extern const char *format_eui64(const unsigned char *eui);
extern const char *format_thousands(unsigned int value);

#endif
