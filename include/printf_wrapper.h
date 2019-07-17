/**
Author: sascha_lammers@gmx.de
*/

#pragma once

#if defined(ESP8266)
#include <Arduino.h>
#include <pgmspace.h>
#endif
#include <stdarg.h>

#ifndef PGM_P
#define PGM_P const char *
#endif

// for the GCC linker option: -Wl,--wrap=snprintf -Wl,--wrap=vsnprintf -Wl,--wrap=snprintf_P -Wl,--wrap=vsnprintf_P
#ifndef PRINTF_WRAPPER_ENABLED
#define PRINTF_WRAPPER_ENABLED                              0
#endif

#ifndef PRINTF_WRAPPER_DEBUG
#define PRINTF_WRAPPER_DEBUG                                0
#endif

#ifndef PRINTF_WRAPPER_ALWAYS_USE_PROGMEM
#if defined(ESP8266)
#define PRINTF_WRAPPER_ALWAYS_USE_PROGMEM                   1
#else
#define PRINTF_WRAPPER_ALWAYS_USE_PROGMEM                   0
#endif
#endif

#ifndef PRINTF_WRAPPER_HAVE_LONG_DOUBLE
#if defined(ESP8266)
#define PRINTF_WRAPPER_HAVE_LONG_DOUBLE                     0
#else
#define PRINTF_WRAPPER_HAVE_LONG_DOUBLE                     1
#endif
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

#ifdef __cplusplus
extern "C" {
#endif

#if defined(ESP8266)
void sprintf_wrapper_info(Print &print);
#endif

int __wrap_snprintf(char *buffer, size_t size, const char *format, ...);
int __wrap_vsnprintf(char *buffer, size_t size, const char *format, va_list va);
int __wrap_snprintf_P(char *buffer, size_t size, PGM_P format,  ...);
int __wrap_vsnprintf_P(char *buffer, size_t size, PGM_P format, va_list va);

#ifdef __cplusplus
}
#endif
