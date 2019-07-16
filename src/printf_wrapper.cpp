/**
Author: sascha_lammers@gmx.de
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <algorithm>
#include <type_traits>
#include "printf_wrapper.h"

#if _WIN32

#define __ALWAYS_INLINE__

#include <Arduino_compat.h>
// return value not evaluated
uint8_t pgm_read_byte_func(const void *ptr) {
    return *(uint8_t *)ptr;
}

#else


//#define __ALWAYS_INLINE__ __attribute__((optimize("-O3"), always_inline))
#define __ALWAYS_INLINE__

#include <Arduino.h>
#if PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT
#include <ESP8266WiFi.h>
#endif
#define pgm_read_byte_func              pgm_read_byte_inlined

#endif

#ifndef __PRINTF_DATA_INTEGRITY_CHECK
#if _WIN32
#define __PRINTF_DATA_INTEGRITY_CHECK   1
#else
#define __PRINTF_DATA_INTEGRITY_CHECK   0
#endif
#endif

#if __PRINTF_DATA_INTEGRITY_CHECK

//#define __PRINTF_VALIDATE               ::printf("%s:%u\n", strrchr(__FILE__, '\\') + 1, __LINE__); __printf->__validate
#define __PRINTF_VALIDATE               __printf->__validate
#define __PRINTF_COPY_BUFFER(buffer)    printf.__copyBuffer(buffer)

#else

#define __PRINTF_VALIDATE()
#define __PRINTF_COPY_BUFFER(buffer)

#endif

#define UNSIGNEDTOA_MAX_STR_LEN(t)      (sizeof(t) * 8 / 3 + 1)
#define POINTER_MAX_STR_LEN             (sizeof(void *) << 1) // hex
#define FLOAT_MAX_STR_LEN               56 // limited to 55 digits
#define IP_MAX_STR_LEN                  (4 * 3 + 3)
#define IPv6_MAX_STR_LEN                (8 * 4 + 7)
#define MAC_MAX_STR_LEN                 (6 * 2 + 5)

#define FORMAT_INT_STRLEN               17 // %+065535.65535llX

#define SAFE_SIZE(n)                    ((n + 8) & ~7)

#define dtostrf_s(n, w, p, c, b, bs) _dtostrf_s(n, w, p, c, b, bs)

#if __PRINTF_DATA_INTEGRITY_CHECK

#include <assert.h>
#define __PRINTF_ASSERT(expr)  assert(expr)

class PrintfWrapper;

static const char *__buffer;
static const char *__endBuffer;
static PrintfWrapper *__printf;

#else

#define __PRINTF_ASSERT(expr)

#endif

typedef uint32_t printf_format_struct_t;
typedef uint16_t printf_size_t;

#define PRECISION_NONE      UINT16_MAX
#define PRECISION_SHORTEST  (UINT16_MAX - 1)
#define STRLEN_NONE         UINT16_MAX

typedef enum : printf_format_struct_t {

    SPECIFIER_INT = 0,
    SPECIFIER_UINT,
    SPECIFIER_OCTAL = SPECIFIER_UINT,
    SPECIFIER_HEX = SPECIFIER_UINT,
    SPECIFIER_FLOAT,
    SPECIFIER_EXP,
    SPECIFIER_CHAR,
    SPECIFIER_STRING,
#if PRINTF_WRAPPER_HAVE_PSTRING
    SPECIFIER_PSTRING,
#endif
    SPECIFIER_POINTER,
#if PRINTF_WRAPPER_HAVE_N
    SPECIFIER_N,
#endif
#if PRINTF_WRAPPER_HAVE_STRING_OBJECT
    SPECIFIER_STRING_OBJECT,
#endif
#if PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT
    SPECIFIER_IPADDRESS_OBJECT,
#endif

} PrintfSpecifier_enum;

typedef enum : printf_format_struct_t {

    MODIFIER_NONE = 0,
#if PRINTF_WRAPPER_HAVE_IPv4
    MODIFIER_IPv4_ADDR,
#endif
#if PRINTF_WRAPPER_HAVE_MAC_ADDRESS
    MODIFIER_MAC_ADDR,
#endif
#if PRINTF_WRAPPER_HAVE_IPv6
    MODIFIER_IPv6_ADDR,
#endif

} PrintfModifier_enum;

typedef enum : uint8_t {
    STATE_BEGIN = 0,
    STATE_ERROR = STATE_BEGIN,
    STATE_END = STATE_BEGIN,
    STATE_FLAGS,
    STATE_WIDTH,
    STATE_PRECISION,
    STATE_PRECISION_DIGITS,
    STATE_LENGTH,
    STATE_SPECIFIER
} ParserState_enum;

typedef struct {
    PrintfSpecifier_enum specifier : 4;
    printf_format_struct_t modifier : 2;

    printf_format_struct_t flagsLeftJustify : 1;
    printf_format_struct_t flagsPreceedPlus : 1;
    printf_format_struct_t flagsInsertSpace : 1;
    printf_format_struct_t flagsHash : 1;
    printf_format_struct_t flagsZeroPadded : 1;
    printf_format_struct_t flagsUpperCase : 1;
    printf_format_struct_t flagsHex : 1;
    printf_format_struct_t flagsOctal : 1;

#if PRINTF_WRAPPER_HAVE_SHORT_INT
    printf_format_struct_t lengthHalfHalf : 1;
    printf_format_struct_t lengthHalf : 1;
#endif
    printf_format_struct_t lengthLong : 1;
    printf_format_struct_t lengthLongLong : 1;
#if PRINTF_WRAPPER_HAVE_LONG_DOUBLE
    printf_format_struct_t lengthLongDouble : 1;
#endif

    printf_size_t width;
    printf_size_t precision;

} PrintfFormat_t;

typedef uint8_t (*ReadCharFunc_t)(const void *ptr);
typedef void *(*MemCopyFunc_t)(void *dst, const void *src, size_t len);

typedef struct {

    ParserState_enum state;
    printf_size_t size;
    printf_size_t outputLength;
    char *writePtr;
    bool formatPStr;

} PrintfParser_t;

class PrintfWrapper {
public:
    PrintfWrapper() {
#if __PRINTF_DATA_INTEGRITY_CHECK
        bufferCopy = nullptr;
#endif
        clearFormat();
        parser.state = STATE_BEGIN;
    }

    void setup(printf_size_t size, char *buffer, bool formatPStr) {
        parser.size = size;
#if __PRINTF_DATA_INTEGRITY_CHECK
        bufferSize = size;
        parser.writePtr = __initValidation();
#else
        parser.writePtr = buffer;
#endif
        if (parser.size) {
            parser.size--; // reserved for NUL
        }
        parser.formatPStr = formatPStr;
        if (formatPStr) {
            getChar = pgm_read_byte_func;
            memCopy = memcpy_P;
        } else{
            getChar = _getChar;
            memCopy = memcpy;
        }
    }

    __ALWAYS_INLINE__ inline void clearFormat() {
        memset(&format, 0, sizeof(format));
#if PRECISION_NONE
        format.precision = PRECISION_NONE;
#endif
    }

    __ALWAYS_INLINE__ inline void beginParser(const char *formatStr) {
        clearFormat();
        parser.state = STATE_FLAGS;
    }

    __ALWAYS_INLINE__ inline void parserContinue(const char *&formatStr) {
        formatStr++;
    }

private:
    template <typename T>
    static uint8_t _dtostrf_s(T number, signed char width, unsigned char prec, char padding, char *s, uint8_t size) {
        bool negative = false;

        if (isnan(number)) {
            strncpy(s, "nan", size);
            return 3;
        }
        if (isinf(number)) {
            strncpy(s, "inf", size);
            return 3;
        }

        char* out = s;
        size--; // NUL byte

        int fillme = width; // how many cells to fill for the integer part
        if (prec > 0) {
            fillme -= (prec+1);
        }

        // Handle negative numbers
        if (number < 0.0) {
            negative = true;
            fillme--;
            number = -number;
        }

        // Round correctly so that print(1.999, 2) prints as "2.00"
        // I optimized out most of the divisions
        T rounding = 2.0;
        for (uint8_t i = 0; i < prec; ++i)
            rounding *= 10.0;
        rounding = 1.0 / rounding;

        number += rounding;

        // Figure out how big our number really is
        T tenpow = 1.0;
        int digitcount = 1;
        while (number >= 10.0 * tenpow) {
            tenpow *= 10.0;
            digitcount++;
        }

        number /= tenpow;
        fillme -= digitcount;

        // Pad unused cells with spaces
        while (fillme-- > 0) {
            if (!--size) {
                *out = 0;
                return out - s;
            }
            *out++ = padding;
        }

        // Handle negative sign
        if (negative) {
            if (!--size) {
                *out = 0;
                return out - s;
            }
            *out++ = '-';
        }

        // Print the digits, and if necessary, the decimal point
        digitcount += prec;
        int8_t digit = 0;
        while (digitcount-- > 0) {
            digit = (int8_t)number;
            if (digit > 9) {
                digit = 9; // insurance
            }
            if (!--size) {
                break;
            }
            *out++ = (char)('0' | digit);
            if ((digitcount == prec) && (prec > 0)) {
                if (!--size) {
                    break;
                }
                *out++ = '.';
            }
            number -= digit;
            number *= 10.0;
        }

        // make sure the string is terminated
        *out = 0;
        return out - s;

    }

    void _writePaddedLongString(char *str, char *valPtr) {
        if (format.precision != PRECISION_NONE || format.width) {
            printf_size_t len = (printf_size_t)strlen(valPtr);
            if (len < format.precision) {
                printf_size_t padding = 0;
                printf_size_t zero_padding = 0;
                if (format.precision != PRECISION_NONE) {
                    zero_padding = format.precision - len;
                    len += zero_padding;
                }
                len += (valPtr - str);
                if (len < format.width) {
                    if (format.flagsZeroPadded && !format.flagsLeftJustify && format.precision == PRECISION_NONE) {
                        zero_padding = format.width - len;
                    }
                    else {
                        padding = format.width - len;
                    }
                }
                if (!format.flagsLeftJustify) {
                    _writeCharRepeat(' ', padding);
                }
                char *src = str;
                while (src < valPtr) { // copy flags like +, -, 0x ...
                    writeChar(*src++);
                }
                // add 0 padding
                _writeCharRepeat('0', zero_padding);
                // write value without truncating it
                format.precision = PRECISION_NONE;
                _writeRawString(valPtr);

                if (format.flagsLeftJustify) {
                    _writeCharRepeat(' ', padding);
                }
            }
        } else {
            _writeRawString(str);
        }
    }

public:
    template <typename T, typename UT>
    void prepareLong(T value) {
        char buffer[SAFE_SIZE(UNSIGNEDTOA_MAX_STR_LEN(sizeof(value)))];
        char *ptr = buffer;
        _addIntFlags(ptr, value);
        // GCC does not like the T in std::make_unsigned<T>::type
        // error: need 'typename' before 'std::make_unsigned<_IntType>::type' because 'std::make_unsigned<_IntType>' is a dependent scope
        // typedef std::make_unsigned<T>::type UTT;
        // auto test_test = (UTT)value;
        _unsignedtoa_len((UT)value, ptr, _getRadix(), format.flagsUpperCase ? HEX_UPPERCASE : HEX_LOWECASE);
        _writePaddedLongString(buffer, ptr);
    }

    template <typename T>
    void prepareDouble(T value) {
        char buffer[FLOAT_MAX_STR_LEN];
        uint8_t len;
        len = dtostrf_s(value, format.flagsLeftJustify ? 0 : format.width, (format.precision != PRECISION_NONE) ? format.precision : 6/*default*/, format.flagsZeroPadded ? '0' : ' ', buffer, sizeof(buffer));
        if (!format.flagsLeftJustify) {
            format.width = 0;
        }
        _writeString(buffer, len);
    }

    __ALWAYS_INLINE__ inline void prepareString(const char *str) {
        _writeString(str);
    }

#if PRINTF_WRAPPER_HAVE_PSTRING
    __ALWAYS_INLINE__ inline void preparePString(PGM_P str) {
        _writePString(str);
    }

#endif

    __ALWAYS_INLINE__ inline void prepareChar(char ch) {
        writeChar(ch);
    }

    void preparePointer(const void *ptr) {
        char buffer[POINTER_MAX_STR_LEN + 1];
        uint8_t len = _unsignedtoa_len((size_t)ptr, buffer, 16, HEX_UPPERCASE);
        if (len < POINTER_MAX_STR_LEN) {
            _writeCharRepeat('0', POINTER_MAX_STR_LEN - len);
        }
        _writeRawString(buffer);
    }

#if PRINTF_WRAPPER_HAVE_N
    void prepareN(void *ptr) {
        if (format.lengthLongLong) {
            (*(long long *)ptr) = parser.outputLength;
        }
        else if (format.lengthLong) {
            (*(long *)ptr) = parser.outputLength;
        }
#if PRINTF_WRAPPER_HAVE_SHORT_INT
        else if (format.lengthHalfHalf) {
            (*(int8_t *)ptr) = (int8_t)parser.outputLength;
        }
        else if (format.lengthHalf) {
            (*(int16_t *)ptr) = (int16_t)parser.outputLength;
        }
#endif
        else {
            (*(signed int *)ptr) = parser.outputLength;
        }
    }
#endif

private:
    static const char HEX_UPPERCASE = 'A' - 10;
    static const char HEX_LOWECASE = 'a' - 10;

    __ALWAYS_INLINE__ inline char _nibble2hex(uint8_t value, char hex) {
        if (value <= 9) {
            return value + '0';
        }
        else {
            return value + hex;
        }
    }

    __ALWAYS_INLINE__ inline void _addHexByte(char *&ptr, uint8_t value, char hex) {
        *ptr++ = _nibble2hex(value >> 4, hex);
        *ptr++ = _nibble2hex(value & 0xf, hex);
    }

    template <typename T>
    uint8_t _unsignedtoa_len(T value, char *buf, uint8_t radix, char hex = HEX_LOWECASE) {
        char tmp[UNSIGNEDTOA_MAX_STR_LEN(T) + 1];
        char *tp = tmp;
        T v = value;
        char *sp;
        __PRINTF_ASSERT(radix == 10 || radix == 8 || radix == 16);

        while (v || tp == tmp) {
            uint8_t i = v % radix;
            v = v / radix;
            *tp++ = _nibble2hex(i, hex);
        }

        sp = buf;
        while (tp > tmp) {
            *sp++ = *--tp;
        }
        *sp = 0;
        return sp - buf;
    }

public:
#if PRINTF_WRAPPER_HAVE_IPv6
    void prepareIPv6Address(const uint8_t *addr) {
        char buf[IPv6_MAX_STR_LEN + 1];
        char *ptr = buf;
        char hex = format.flagsUpperCase ? HEX_UPPERCASE : HEX_LOWECASE;
        uint8_t count = 0;
        while(true) {
            if (format.flagsZeroPadded) {
                _addHexByte(ptr, *addr++, hex);
                _addHexByte(ptr, *addr++, hex);
            }
            else {
                if (*(uint16_t *)addr == 0) {
                    if (count == 0) {
                        *ptr++ = ':';
                    }
                    do {
                        addr += 2;
                        if (++count == 8) {
                            *ptr++ = ':';
                            goto exitLoops;
                        }
                    } while (*(uint16_t *)addr == 0);
                    *ptr++ = ':';
                }
                uint16_t value = (*addr++ << 8);
                value |= *addr++;
                ptr += _unsignedtoa_len(value, ptr, 16, hex);
            }
            if (++count == 8) {
                break;
            }
            *ptr++ = ':';
        }
exitLoops:
        *ptr = 0;
        _writeString(buf, ptr - buf);
    }
#endif

#if PRINTF_WRAPPER_HAVE_IPv4
    void prepareIPAddress(uint32_t addr) {
        char buf[IP_MAX_STR_LEN + 1];
        char *ptr = buf;
        uint8_t count = 0;
        while(true) {
            _unsignedtoa_len(((uint8_t *)&addr)[count++], ptr, 10);
            ptr += strlen(ptr);
            if (count == 4) {
                break;
            }
            *ptr++ = '.';
        }
        _writeString(buf, ptr - buf);
    }
#endif

#if PRINTF_WRAPPER_HAVE_MAC_ADDRESS
    void prepareMacAddress(const uint8_t *mac) {
        char buf[MAC_MAX_STR_LEN + 1];
        char *ptr = buf;
        char sch = format.width != 2 ? (format.flagsHash ? '-' : ':') : 0;
        char hex = format.flagsUpperCase ? HEX_UPPERCASE : HEX_LOWECASE;
        uint8_t count = 0;
        while(true) {
            _addHexByte(ptr, *mac++, hex);
            if (++count == 6) {
                break;
            }
            if (sch) {
                *ptr++ = sch;
            }
        }
        *ptr = 0;
        _writeString(buf, ptr - buf);
    }
#endif

    __ALWAYS_INLINE__ inline void writeChar(uint8_t ch) {
        if (parser.size) {
            parser.size--;
            *parser.writePtr++ = ch;
        }
        parser.outputLength++;
    }

    void copyFormatString(const char *str, printf_size_t len) {

        parser.outputLength += len;
        if (len < parser.size) {
            memCopy(parser.writePtr, str, len);
            parser.writePtr += len;
            parser.size -= len;
        } else {
            memCopy(parser.writePtr, str, parser.size);
            parser.writePtr += parser.size;
            parser.size = 0;
        }
        __PRINTF_VALIDATE();
    }

public:
    PrintfFormat_t format;
    PrintfParser_t parser;

    ReadCharFunc_t getChar;
    MemCopyFunc_t memCopy;

private:
#if PRINTF_WRAPPER_HAVE_PSTRING

    void _writePString(PGM_P str) {
        uint8_t ch;
        if (format.precision != PRECISION_NONE || format.width) {

            char *startPtr = parser.writePtr;
            printf_size_t startsize = parser.size;
            printf_size_t len = 0;

            // precision block
            if (format.precision != PRECISION_NONE) {
                while (format.precision-- && (ch = pgm_read_byte(str++))) {       // strncpy + strlen
                    if (parser.size) {
                        parser.size--;
                        *parser.writePtr++ = ch;
                    }
                    else {
                        do {
                            len++;
                        } while (format.precision-- && pgm_read_byte(++str));
                        break;
                    }
                    len++;
                }
            }
            else {
                while ((ch = pgm_read_byte(str++))) {                      // strcpy + strlen
                    if (parser.size) {
                        parser.size--;
                        *parser.writePtr++ = ch;
                    } else {
                        do {
                            len++;
                        } while (pgm_read_byte(++str));
                        break;
                    }
                    len++;
                }
            }

            // width block
            if (len < format.width) {  // any padding required?
                if (format.flagsLeftJustify) {
                    format.width -= (printf_size_t)len;
                    if (format.width < parser.size) {
                        memset(parser.writePtr, ' ', format.width);         // padding is always space
                        parser.writePtr += format.width;
                        parser.size -= format.width;
                    } else if (parser.size) {                               // max. buffer size, limit fill length
                        memset(parser.writePtr, ' ', parser.size);          // padding is always space
                        parser.writePtr += parser.size;
                        parser.size = 0;
                    }
                    len += format.width;
                    __PRINTF_VALIDATE();
                }
                else {                                                  // right aligned
                    char padding = (format.flagsZeroPadded) ? '0' : ' ';
                    printf_size_t move = format.width - len;
                    if (move >= startsize) {                         // fill only, max. buffer size reached
                        memset(startPtr, padding, startsize);
                        parser.writePtr = startPtr + startsize;
                        parser.size = 0;
                        __PRINTF_VALIDATE();
                    }
                    else if (move + len > startsize) {               // move part of the PROGMEM string and fill the gap
                        memmove(startPtr + move, startPtr, len - (format.width - startsize));
                        memset(startPtr, padding, move);
                        startsize -= len;
                        parser.writePtr += startsize;
                        parser.size -= startsize;
                        __PRINTF_VALIDATE();
                    }
                    else {                                              // enough space, move the entire PROGMEM string and fill the gap
                        memmove(startPtr + move, startPtr, len);
                        memset(startPtr, padding, move);
                        parser.writePtr += move;
                        parser.size -= move;
                        __PRINTF_VALIDATE();
                    }
                    len += move;
                }
            }
            parser.outputLength += len;

        }
        else {
            while ((ch = pgm_read_byte(str++))) {
                if (parser.size) {
                    parser.size--;
                    *parser.writePtr++ = ch;
                } else {
                    parser.outputLength += (printf_size_t)strlen_P(str) + 1;
                    break;
                }
                parser.outputLength++;
            }
            __PRINTF_VALIDATE();
        }
    }
#endif

    void _writeCharRepeat(uint8_t ch, printf_size_t len) {

        parser.outputLength += len;
        if (len < parser.size) {
            memset(parser.writePtr, ch, len);
            parser.writePtr += len;
            parser.size -= len;
        } else {
            memset(parser.writePtr, ch, parser.size);
            parser.writePtr += parser.size;
            parser.size = 0;
        }
        __PRINTF_VALIDATE();
    }

    // choose which write method to use, either with padding or without
    void _writeString(const char *str, printf_size_t len = STRLEN_NONE) {
        if (format.width) {
            _writeStringPadded(str, len);
        }
        else {
            _writeRawString(str);
        }
    }

    // write string left or right aligned to buffer
    void _writeStringPadded(const char *str, printf_size_t slen = STRLEN_NONE) {

        if (format.flagsLeftJustify) {
            auto len = _writeRawString(str);
            if (len < format.width) {
                format.width -= len;
                _writeCharRepeat(' ', format.width);
            }
        }
        else {
            auto len = slen != STRLEN_NONE ? slen : _strlen(str);
            if (len < format.width) {
                format.width -= (printf_size_t)len;
                _writeCharRepeat(format.flagsZeroPadded ? '0' : ' ', format.width);
                //_writeCharRepeat(' ', format.width);
            }
            _writeRawString(str);
        }
        __PRINTF_VALIDATE();
    }

    __ALWAYS_INLINE__ inline static uint8_t _getChar(const void *ptr) {
        return *(uint8_t *)ptr;
    }

    inline char _getFloatSpecifier() {
        if (format.specifier == SPECIFIER_EXP) {
            return format.flagsUpperCase ? 'E' : 'e';
        }
        else if (format.flagsHex) {
            return format.flagsUpperCase ? 'A' : 'a';
        }
        else {
            return format.flagsUpperCase ? 'F' : 'f';
        }
    }

    void _addFormatFlags(char *&ptr) {
        if (format.flagsLeftJustify) {
            *ptr++ = '-';
        }
        if (format.flagsPreceedPlus) {
            *ptr++ = '+';
        }
        else if (format.flagsInsertSpace) {
            *ptr++ = ' ';
        }
        if (format.flagsZeroPadded) {
            *ptr++ = '0';
        }
        if (format.width) {
            ptr += _unsignedtoa_len(format.width, ptr, 6, 10);
        }
        if (format.precision != PRECISION_NONE) {
            *ptr++ = '.';
            ptr += _unsignedtoa_len(format.precision, ptr, 6, 10);
        }
    }

    __ALWAYS_INLINE__ inline uint8_t _getRadix() {
        if (format.flagsHex) {
            return 16;
        }
        else if (format.flagsOctal) {
            return 8;
        }
        return 10;
    }

    // adds '+', '-', ' ', '0x', '0'
    template <typename T>
    void _addIntFlags(char *&ptr, T &value) {
        if (format.flagsHex || format.flagsOctal) {
            format.flagsPreceedPlus = 0;
            if (format.flagsHash) {
                *ptr++ = '0';
                *ptr++ = format.flagsUpperCase ? 'X' : 'x';
            } else if (format.flagsOctal) {
                *ptr++ = '0';
            }
        }
        else {
            if (format.specifier == SPECIFIER_INT) {
                if (value < 0) {
                    value = -value;
                    *ptr++ = '-';
                }
                else if (format.flagsPreceedPlus) {
                        *ptr++ = '+';
                } else if (format.flagsInsertSpace) {
                        *ptr++ = ' ';
                }
            }
        }
    }

    // write string to buffer and truncate it if precision is set
    printf_size_t _writeRawString(const char *str) {

        const char *startPtr = str;
        if (format.precision != PRECISION_NONE) {
            while(format.precision-- && *str) {
                if (parser.size) {
                    parser.size--;
                    *parser.writePtr++ = *str++;
                } else {
                    do {
                        str++;
                    } while (format.precision-- && *str);
                    break;
                }
            }
        }
        else {
            while(*str) {
                if (parser.size) {
                    parser.size--;
                    *parser.writePtr++ = *str++;
                } else {
                    while (*++str) {
                    }
                    break;
                }
            }
        }
        printf_size_t strlen = str - startPtr;
        parser.outputLength += strlen;
        __PRINTF_VALIDATE();
        return strlen;
    }

    // get string length limited by precision
    printf_size_t _strlen(const char *str) {
        const char *ptr = str;
        if (format.precision != PRECISION_NONE) {
            auto precision = format.precision;
            while(precision-- && *ptr) {
                ptr++;
            }
            return ptr - str;
        }
        else {
            return (printf_size_t)strlen(str);
        }
    }

    // get string length limited by precision
    printf_size_t _strlen2(const char *str) {
        return std::min(format.precision, (printf_size_t)strlen(str));
    }


#if __PRINTF_DATA_INTEGRITY_CHECK

public:
    void __validate(int moveWritePtr = 0) {
        __PRINTF_ASSERT(parser.writePtr >= _buffer());
        if (bufferSize == 0) {
            if (!(parser.writePtr == _buffer())) {
                __dump();
            }
            __PRINTF_ASSERT(parser.writePtr == _buffer());
        }
        else {
            if (!(parser.writePtr < _endBuffer())) {
                __dump();
            }
            __PRINTF_ASSERT(parser.writePtr < _endBuffer());
        }
        char *ptr = bufferCopy;
        char *endPtr = ptr + bufferStart;
        while (ptr < endPtr) {
            if ((uint8_t)*ptr != 0xcc) {
                __dump();
                printf("BUFFER pos %d, start %d, ptr %p endPtr %pm\n", ptr - bufferCopy, bufferStart, ptr, endPtr);
            }
            __PRINTF_ASSERT((uint8_t)*ptr == 0xcc);
            ptr++;
        }
        ptr = parser.writePtr + moveWritePtr;
        endPtr = bufferCopy + bufferCopySize;
        while (ptr < endPtr) {
            if ((uint8_t)*ptr != 0xcc) {
                __dump();
                printf("BUFFER pos %d, end %d, ptr %p endPtr %p\n", ptr - bufferCopy, bufferEnd, ptr, endPtr);
            }
            __PRINTF_ASSERT((uint8_t)*ptr == 0xcc);
            ptr++;
        }
    }

    void __copyBuffer(char *buffer) {
        __validate(1);
        memCopy(buffer, bufferCopy + bufferStart, bufferSize);
    }

    ~PrintfWrapper() {
        if (bufferCopy) {
            free(bufferCopy);
        }
    }

private:
    char * __initValidation() {
        __PRINTF_ASSERT(bufferCopy == nullptr);
        bufferCopySize = bufferSize * 2;
        if (bufferCopySize < 1024) {
            bufferCopySize = 1024;
        }
        bufferCopy = (char *)malloc(bufferCopySize);
        bufferStart = (bufferCopySize - bufferSize) / 2;
        bufferEnd = bufferStart + bufferSize;
        memset(bufferCopy, 0xcc, bufferCopySize);
        __buffer = _buffer();
        __endBuffer = _endBuffer();
        return bufferCopy + bufferStart;
    }

    void __dump() {
        printf("format '%s', writePtr = %d\nstate %d\nspace %d(+1)\noutput length %d\n", formatString, parser.writePtr - _buffer(), parser.state, parser.size, parser.outputLength);
    }

    char *_buffer() {
        return bufferCopy + bufferStart;
    }

    char *_endBuffer() {
        return bufferCopy + bufferEnd;
    }

    char *bufferCopy, *bufferPtr;
    int bufferStart, bufferEnd, bufferSize, bufferCopySize;

public:
    const char *formatString;
#endif
};

int __vsnprintf_wrapper_ex(char *buffer, size_t const size, const char *fmtStr, va_list va, bool formatPStr) {

    PrintfWrapper printf;
#if __PRINTF_DATA_INTEGRITY_CHECK
    __printf = &printf;
    printf.formatString = fmtStr;
#endif
#if COMPARE_RESULTS
    va_list va_copy;
    va_copy(va_copy, va);
    auto fmtStr_copy = fmtStr;
#endif

    printf.setup((printf_size_t)size, buffer, formatPStr);

    auto &state = printf.parser.state;
    char currentChar;
    while ((currentChar = printf.getChar(fmtStr))) {

        if (state == STATE_BEGIN) {

            if (currentChar == '%') {
                if (printf.getChar(fmtStr + 1) == '%') { // escaped %
                    printf.writeChar('%');
                    fmtStr += 2;
                }
                else { // format string begins
                    printf.beginParser(fmtStr);
                    fmtStr++;
                }
            }
            else {
                const char *endPtr = fmtStr;
                while(currentChar != 0 && currentChar != '%') {
                    currentChar = printf.getChar(++endPtr);
                }
                printf.copyFormatString(fmtStr, endPtr - fmtStr);
                fmtStr = endPtr;
                __PRINTF_VALIDATE();
            }

        } else {
            if (state == STATE_FLAGS) {
                switch (currentChar) {
                case '-':
                    printf.format.flagsLeftJustify = 1;
                    break;
                case '+':
                    printf.format.flagsPreceedPlus = 1;
                    break;
                case '#':
                    printf.format.flagsHash = 1;
                    break;
                case ' ':
                    printf.format.flagsInsertSpace = 1;
                    break;
                case '0':
                    printf.format.flagsZeroPadded = 1;
                    break;
#if PRINTF_WRAPPER_HAVE_MAC_ADDRESS
                case 'M':
                    printf.format.modifier = MODIFIER_MAC_ADDR;
                    break;
#endif
#if PRINTF_WRAPPER_HAVE_IPv4
                case 'j':
                    printf.format.modifier = MODIFIER_IPv4_ADDR;
                    break;
#endif
#if PRINTF_WRAPPER_HAVE_IPv6
                case 'J':
                    printf.format.modifier = MODIFIER_IPv6_ADDR;
                    break;
#endif
                default:
                    state = STATE_WIDTH;
                    goto nextStateWidth;
                }
                printf.parserContinue(fmtStr);
                continue;
            }
nextStateWidth:
            if (state == STATE_WIDTH) {
                if (isdigit(currentChar)) {
                    printf.format.width = (printf.format.width * 10) + (currentChar - '0');
                    printf.parserContinue(fmtStr);
                    continue;
                }
                else {
                    state = STATE_PRECISION;
                }
            }
            if (state == STATE_PRECISION) {
                if (currentChar == '.') {
                    printf.format.precision = 0;
                    state = STATE_PRECISION_DIGITS;
                    printf.parserContinue(fmtStr);
                    continue;
                }
                else {
                    state = STATE_LENGTH;;
                }
            }
            if (state == STATE_PRECISION_DIGITS) {
                if (currentChar == '.') {
                    printf.parser.state = STATE_ERROR;
                    continue;
                }
                else if (isdigit(currentChar)) {
                    printf.format.precision = (printf.format.precision * 10) + (currentChar - '0');
                    printf.parserContinue(fmtStr);
                    continue;
                }
                else {
                    state = STATE_LENGTH;
                }
            }
            if (state == STATE_LENGTH) {
                switch (currentChar) {
                case 'l':
                    if (printf.format.lengthLong) {
                        printf.format.lengthLong = 0;
                        printf.format.lengthLongLong = 1;
                        state = STATE_SPECIFIER;
                    }
                    else {
                        printf.format.lengthLong = 1;
                    }
                    break;
#if PRINTF_WRAPPER_HAVE_SHORT_INT
                case 'h':
                    if (printf.format.lengthHalf) {
                        printf.format.lengthHalf = 0;
                        printf.format.lengthHalfHalf = 1;
                        state = STATE_SPECIFIER;
                    }
                    else {
                        printf.format.lengthHalf = 1;
                    }
                    break;
#endif
                    default:
                        state = STATE_SPECIFIER;
                        goto nextStateSpecifier;
                }
                printf.parserContinue(fmtStr);
                continue;
            }
nextStateSpecifier:
            if (state == STATE_SPECIFIER) {
                switch (currentChar) {
                case 'i':
                case 'd':
                    printf.format.specifier = SPECIFIER_INT;
                    break;
                case 'u':
                    printf.format.specifier = SPECIFIER_UINT;
                    break;
                case 'o':
                    printf.format.specifier = SPECIFIER_OCTAL;
                    printf.format.flagsOctal = 1;
                    break;
                case 'x':
                    printf.format.specifier = SPECIFIER_HEX;
                    printf.format.flagsHex = 1;
                    break;
                case 'X':
                    printf.format.specifier = SPECIFIER_HEX;
                    printf.format.flagsHex = 1;
                    printf.format.flagsUpperCase = 1;
                    break;
                case 'p':
                    printf.format.specifier = SPECIFIER_POINTER;
                    break;
                case 'f':
                    printf.format.specifier = SPECIFIER_FLOAT;
                    break;
                case 'a':
                    printf.format.specifier = SPECIFIER_FLOAT;
                    printf.format.flagsHex = 1;
                    break;
                case 'A':
                    printf.format.specifier = SPECIFIER_FLOAT;
                    printf.format.flagsHex = 1;
                    printf.format.flagsUpperCase = 1;
                    break;
                case 'F':
                    printf.format.specifier = SPECIFIER_FLOAT;
                    printf.format.flagsUpperCase = 1;
                    break;
                case 'e':
                    printf.format.specifier = SPECIFIER_EXP;
                    break;
                case 'E':
                    printf.format.specifier = SPECIFIER_EXP;
                    printf.format.flagsUpperCase = 1;
                    break;
                case 's':
                    printf.format.specifier = SPECIFIER_STRING;
                    break;
                case 'c':
                    printf.format.specifier = SPECIFIER_CHAR;
                    break;
#if PRINTF_WRAPPER_HAVE_PSTRING
                case 'P':
                    printf.format.specifier = SPECIFIER_PSTRING;
                    break;
#endif
#if HAVE_PRINTF_WRAPPER_BINARY
                case 'B':
                    printf.format.specifier = SPECIFIER_BINARY;
                    break;
#endif
#if PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT
                case 'I':
                    printf.format.specifier = SPECIFIER_IPADDRESS_OBJECT;
                    break;
#endif
#if PRINTF_WRAPPER_HAVE_STRING_OBJECT
                case 'S':
                    printf.format.specifier = SPECIFIER_STRING_OBJECT;
                    break;
#endif
#if PRINTF_WRAPPER_HAVE_N
                case 'n':
                    printf.format.specifier = SPECIFIER_N;
                    break;
#endif
                default:
                    printf.parser.state = STATE_ERROR;
                    continue;
                }
                state = STATE_END; // end of format
            }
            if (state != STATE_END) {
                printf.parserContinue(fmtStr);
                continue;
            }
            fmtStr++;

            PrintfSpecifier_enum specifier = printf.format.specifier;

            if (specifier <= SPECIFIER_HEX) {
                // SPECIFIER_INT
                // SPECIFIER_UINT
                // SPECIFIER_OCTAL
                // SPECIFIER_HEX

#if PRINTF_WRAPPER_HAVE_SHORT_INT
                if (printf.format.lengthHalfHalf) {
                    printf.prepareLong<int8_t, uint8_t>((int8_t)va_arg(va, int));
                }
                else if (printf.format.lengthHalf) {
                    printf.prepareLong<int16_t, uint16_t>((int16_t)va_arg(va, int));
                }
                else
#endif
                if (printf.format.lengthLong) {
                    printf.prepareLong<long, unsigned long>(va_arg(va, long));
                }
                else if (printf.format.lengthLongLong) {
                    printf.prepareLong<long long, unsigned long>(va_arg(va, long long));
                } else {
#if UINT_MAX == ULONG_MAX
                    // GCC creates extra code even if the size of the types match
                    printf.prepareLong<long, unsigned long>((long)va_arg(va, int));
#else
                    printf.prepareLong<int, unsigned int>(va_arg(va, int));
#endif
                }
            }
            else if (specifier == SPECIFIER_STRING) {
                printf.prepareString(va_arg(va, const char *));
            }
#if PRINTF_WRAPPER_HAVE_PSTRING
            else if (specifier == SPECIFIER_PSTRING) {
                printf.preparePString(va_arg(va, PGM_P));
            }
#endif
            else if (specifier == SPECIFIER_CHAR) {
                printf.writeChar(va_arg(va, int));
            }
            else if (specifier == SPECIFIER_POINTER) {
                printf.preparePointer(va_arg(va, const void *));
            }
#if PRINTF_WRAPPER_HAVE_N
            else if (specifier == SPECIFIER_N) {
                printf.prepareN(va_arg(va, void *));
            }
#endif
#if PRINTF_WRAPPER_HAVE_STRING_OBJECT
            else if (specifier == SPECIFIER_STRING_OBJECT) {
                printf.prepareString(va_arg(va, String *)->c_str());
            }
#endif
#if PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT
            else if (specifier == SPECIFIER_IPADDRESS_OBJECT) {
                auto str = va_arg(va, IPAddress *)->toString();
                printf.prepareString(str.c_str());
            }
#endif
            else if (specifier == SPECIFIER_FLOAT || specifier == SPECIFIER_EXP) {
                if (printf.format.flagsHex) {
#if PRINTF_WRAPPER_HAVE_IPv4
                    if (printf.format.modifier == MODIFIER_IPv4_ADDR) {
                        printf.prepareIPAddress(va_arg(va, uint32_t));
                        continue;
                    }
#if PRINTF_WRAPPER_HAVE_MAC_ADDRESS
                    else
#endif
#endif
#if PRINTF_WRAPPER_HAVE_MAC_ADDRESS
                    if (printf.format.modifier == MODIFIER_MAC_ADDR) {
                        printf.prepareMacAddress(va_arg(va, const uint8_t *));
                        continue;
                    }
#if PRINTF_WRAPPER_HAVE_IPv6
                    else
#endif
#endif
#if PRINTF_WRAPPER_HAVE_IPv6
                    if (printf.format.modifier == MODIFIER_IPv6_ADDR) {
                        printf.prepareIPv6Address(va_arg(va, const uint8_t *));
                        continue;
                    }
#endif
                }
#if PRINTF_WRAPPER_HAVE_LONG_DOUBLE
                if (printf.format.lengthLongDouble) {
                    printf.prepareDouble(va_arg(va, long double));
                }
                else {
                    printf.prepareDouble(va_arg(va, double));
                }
#else
                printf.prepareDouble(va_arg(va, double));
#endif
            }
#if DBEUG
            else {
                __PRINTF_ASSERT(false);
            }
#endif
            __PRINTF_VALIDATE();
        }
    }
    if (size) { // space for NUL?
        *printf.parser.writePtr = 0;
    }

    __PRINTF_COPY_BUFFER(buffer);

#if COMPARE_RESULTS
    {
        if (size) {
            char *dest = (char *)malloc(size);
            int res = vsnprintf(dest, size, fmtStr_copy, va_copy);
            if (strcmp(buffer, dest) != 0) {
                Serial.printf("printf_wrapper mismatch, size %d, format '%s'\n", size, fmtStr);
                Serial.printf("'%s'\n'%s'\n", dest, buffer);
            }
            free(dest);
        }
    }
#endif

    return printf.parser.outputLength;
}

int __snprintf_wrapper(char *buffer, size_t const size, const char *format, ...) {
    int result;
    va_list arg;
    va_start(arg, format);
    result = __vsnprintf_wrapper_ex(buffer, size, format, arg, false);
    va_end(arg);
    return result;
}

int __vsnprintf_wrapper(char *buffer, size_t const size, const char *format, va_list arg) {
    return __vsnprintf_wrapper_ex(buffer, size, format, arg, false);
}

int __snprintf_wrapper_P(char *buffer, size_t const size, PGM_P format, ...) {
    int result;
    va_list arg;
    va_start(arg, format);
    result = __vsnprintf_wrapper_ex(buffer, size, format, arg, true);
    va_end(arg);
    return result;
}

int __vsnprintf_wrapper_P(char *buffer, size_t const size, PGM_P format, va_list arg) {
    return __vsnprintf_wrapper_ex(buffer, size, format, arg, true);
}



int printf_wrapper(const char *format, ...) {
    char buffer[1024];
    va_list va;
    va_start(va, format);
    int result = __vsnprintf_wrapper(buffer, sizeof(buffer), format, va);
    ::printf("%s", buffer);
    va_end(va);
    return result;
}
