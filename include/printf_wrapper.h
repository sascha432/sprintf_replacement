/**
Author: sascha_lammers@gmx.de
*/

#pragma once

#include <stdarg.h>

#ifndef PGM_P
#define PGM_P const char *
#endif

#ifndef PRINTF_WRAPPER_HAVE_LONG_DOUBLE
#define PRINTF_WRAPPER_HAVE_LONG_DOUBLE                     1
#endif
#ifndef PRINTF_WRAPPER_HAVE_SHORT_INT
#define PRINTF_WRAPPER_HAVE_SHORT_INT                       0
#endif
#ifndef PRINTF_WRAPPER_HAVE_IPv6
#define PRINTF_WRAPPER_HAVE_IPv6                            0
#endif
#ifndef PRINTF_WRAPPER_HAVE_N
#define PRINTF_WRAPPER_HAVE_N                               0
#endif
#ifndef PRINTF_WRAPPER_HAVE_PSTRING
#define PRINTF_WRAPPER_HAVE_PSTRING                         1
#endif
#ifndef PRINTF_WRAPPER_HAVE_IPv4
#define PRINTF_WRAPPER_HAVE_IPv4                            1
#endif
#ifndef PRINTF_WRAPPER_HAVE_MAC_ADDRESS
#define PRINTF_WRAPPER_HAVE_MAC_ADDRESS                     1
#endif
#ifndef PRINTF_WRAPPER_HAVE_STRING_OBJECT
#define PRINTF_WRAPPER_HAVE_STRING_OBJECT                   1
#endif
#ifndef PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT
#define PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT                1
#endif

int printf_wrapper(const char *format, ...);

#ifdef __cplusplus
extern "C" {
#endif

int __snprintf_wrapper(char *buffer, size_t const size, const char *format, ...);
int __vsnprintf_wrapper(char *buffer, size_t const size, const char *format, va_list arg);
int __snprintf_wrapper_P(char *buffer, size_t const size, PGM_P format, ...);
int __vsnprintf_wrapper_P(char *buffer, size_t const size, PGM_P format, va_list arg);

#ifdef __cplusplus
}
#endif
