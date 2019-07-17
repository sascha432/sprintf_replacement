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

#if PRINTF_WRAPPER_ENABLED

extern "C" int __real_snprintf(char *buffer, size_t size, const char *format, ...);

#else

#define __real_snprintf     snprintf

#endif


#if _WIN32

#define __ALWAYS_INLINE__

#include <Arduino_compat.h>
// return value not evaluated
uint8_t pgm_read_byte_func(const void *ptr) {
    return *(uint8_t *)ptr;
}

#else

#define __ALWAYS_INLINE__               __attribute__((optimize("-O3"), always_inline))
//#define __ALWAYS_INLINE__

#include <Arduino.h>
#if PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT
#include <ESP8266WiFi.h>
#endif
#define pgm_read_byte_func              pgm_read_byte_inlined

#endif

#if PRINTF_WRAPPER_ALWAYS_USE_PROGMEM
#define PRINTF_MEMCPY(dst, src, len)    memcpy_P(dst, src, len)
#define PRINTF_GETCHAR(ptr)             pgm_read_byte(ptr)
#define PRINTF_PSTRING_FORMAT_ARG
#define PRINTF_PSTRING_FORMAT_CSTR(fmt)     fmt
#define PRINTF_PSTRING_FORMAT_PSTR(fmt)     fmt
#else
#define PRINTF_MEMCPY(dst, src, len)        memCopy(dst, src, len)
#define PRINTF_GETCHAR(ptr)                 getChar(ptr)
#define PRINTF_PSTRING_FORMAT_ARG           , bool formatIsPString
#define PRINTF_PSTRING_FORMAT_CSTR(fmt)     false, fmt
#define PRINTF_PSTRING_FORMAT_PSTR(fmt)     true, fmt
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
#define __PRINTF_COPY_BUFFER()         __copyBuffer()

#else

#define __PRINTF_VALIDATE()
#define __PRINTF_COPY_BUFFER()

#endif

#define UNSIGNEDTOA_MAX_STR_LEN(t)      (sizeof(t) * 8 / 3 + 1)
#define POINTER_MAX_STR_LEN             (sizeof(void *) << 1) // hex
#define FLOAT_MAX_STR_LEN               56 // limited to 55 digits
#define IP_MAX_STR_LEN                  (4 * 3 + 3)
#define IPv6_MAX_STR_LEN                (8 * 4 + 7)
#define MAC_MAX_STR_LEN                 (6 * 2 + 5)

#define FORMAT_INT_STRLEN               17 // %+065535.65535llX

#define SAFE_SIZE(n)                    ((n + 8) & ~7)

#ifndef ULLONG_MAX
#define ULLONG_MAX 0xffffffffffffffffULL
#endif

#if __PRINTF_DATA_INTEGRITY_CHECK

class PrintfWrapper;

static PrintfWrapper *__printf;

#include <assert.h>
#define __PRINTF_ASSERT(expr)               assert(expr)

#else

#define __PRINTF_ASSERT(expr)

#endif

typedef uint32_t printf_format_struct_t;
typedef uint16_t printf_size_t;

#define PRECISION_NONE      UINT16_MAX
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

/*

void ftoa(float f, char *str, uint8_t precision) {
uint8_t i, j, divisor = 1;
int8_t log_f;
int32_t int_digits = (int)f;             //store the integer digits
float decimals;
char s1[12];

memset(str, 0, sizeof(s));
memset(s1, 0, 10);

if (f < 0) {                             //if a negative number
str[0] = '-';                          //start the char array with '-'
f = abs(f);                            //store its positive absolute value
}
log_f = ceil(log10(f));                  //get number of digits before the decimal
if (log_f > 0) {                         //log value > 0 indicates a number > 1
if (log_f == precision) {              //if number of digits = significant figures
f += 0.5;                            //add 0.5 to round up decimals >= 0.5
itoa(f, s1, 10);                     //itoa converts the number to a char array
strcat(str, s1);                     //add to the number string
}
else if ((log_f - precision) > 0) {    //if more integer digits than significant digits
i = log_f - precision;               //count digits to discard
divisor = 10;
for (j = 0; j < i; j++) divisor *= 10;    //divisor isolates our desired integer digits
f /= divisor;                             //divide
f += 0.5;                            //round when converting to int
int_digits = (int)f;
int_digits *= divisor;               //and multiply back to the adjusted value
itoa(int_digits, s1, 10);
strcat(str, s1);
}
else {                                 //if more precision specified than integer digits,
itoa(int_digits, s1, 10);            //convert
strcat(str, s1);                     //and append
}
}

else {                                   //decimal fractions between 0 and 1: leading 0
s1[0] = '0';
strcat(str, s1);
}

if (log_f < precision) {                 //if precision exceeds number of integer digits,
decimals = f - (int)f;                 //get decimal value as float
strcat(str, ".");                      //append decimal point to char array

i = precision - log_f;                 //number of decimals to read
for (j = 0; j < i; j++) {              //for each,
decimals *= 10;                      //multiply decimals by 10
if (j == (i-1)) decimals += 0.5;     //and if it's the last, add 0.5 to round it
itoa((int)decimals, s1, 10);         //convert as integer to character array
strcat(str, s1);                     //append to string
decimals -= (int)decimals;           //and remove, moving to the next
}
}
}

char * dtostrf(double number, signed char width, unsigned char prec, char *s) {
bool negative = false;

if (isnan(number)) {
strcpy(s, "nan");
return s;
}
if (isinf(number)) {
strcpy(s, "inf");
return s;
}

char* out = s;

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
double rounding = 2.0;
for (uint8_t i = 0; i < prec; ++i)
rounding *= 10.0;
rounding = 1.0 / rounding;

number += rounding;

// Figure out how big our number really is
double tenpow = 1.0;
int digitcount = 1;
while (number >= 10.0 * tenpow) {
tenpow *= 10.0;
digitcount++;
}

number /= tenpow;
fillme -= digitcount;

// Pad unused cells with spaces
while (fillme-- > 0) {
*out++ = ' ';
}

// Handle negative sign
if (negative) *out++ = '-';

// Print the digits, and if necessary, the decimal point
digitcount += prec;
int8_t digit = 0;
while (digitcount-- > 0) {
digit = (int8_t)number;
if (digit > 9) digit = 9; // insurance
*out++ = (char)('0' | digit);
if ((digitcount == prec) && (prec > 0)) {
*out++ = '.';
}
number -= digit;
number *= 10.0;
}

// make sure the string is terminated
*out = 0;
return s;
}

*/

typedef uint8_t (*ReadCharFunc_t)(const void *ptr);
typedef void *(*MemCopyFunc_t)(void *dst, const void *src, size_t len);

typedef struct {

    ParserState_enum state;
    printf_size_t size;
    printf_size_t outputLength;
    char *writePtr;
#if !PRINTF_WRAPPER_ALWAYS_USE_PROGMEM
    bool formatPStr;
#endif

} PrintfParser_t;

class PrintfWrapper {
public:
    PrintfWrapper() {
#if __PRINTF_DATA_INTEGRITY_CHECK
        bufferCopy = nullptr;
#endif
        clearFormat();
        parser.state = STATE_BEGIN;
        parser.outputLength = 0;
    }

    __ALWAYS_INLINE__ inline void setup(printf_size_t size, char *buffer PRINTF_PSTRING_FORMAT_ARG) {
        parser.size = size;
#if __PRINTF_DATA_INTEGRITY_CHECK
        bufferSize = size;
        orgBuffer = buffer;
        parser.writePtr = __initValidation();
#else
        parser.writePtr = buffer;
#endif
        if (parser.size) {
            parser.size--; // reserved for NUL
        }
#if !PRINTF_WRAPPER_ALWAYS_USE_PROGMEM
        parser.formatPStr = formatIsPString;
        if (formatIsPString) {
            getChar = pgm_read_byte_func;
            memCopy = memcpy_P;
        } else{
            getChar = _getChar;
            memCopy = memcpy;
        }
#endif
    }

    __ALWAYS_INLINE__ inline void clearFormat() {
        memset(&format, 0, sizeof(format));
#if PRECISION_NONE
        format.precision = PRECISION_NONE;
#endif
    }

private:

#if 0
    template <typename T>
    uint8_t _dtostrf_s(T number, uint8_t prec, char *buffer, uint8_t size) { // the size of the buffer needs to be at least 20 byte

        __PRINTF_ASSERT(number >= 0.0);
        __PRINTF_ASSERT(size >= 20);

        if (isnan(number)) {
            strncpy(buffer, "nan", size);
            return 3;
        }
        else if (isinf(number)) {
            strncpy(buffer, "inf", size);
            return 3;
        }
        else if (number > ULLONG_MAX) { // well, now we are done, too much is too much
            strncpy(buffer, "ovf", size);
            return 3;
        }

        //uint64_t frac = (*(uint64_t *)&number) & 0b1111111111111111111111111111111111111111111111111111; // 56 bit
        //uint16_t exp = ((*(uint16_t *)&(((uint8_t *)&number)[6])) & 0b0111111111110000) >> 8; // next 16bit
        //uint8_t is_signed = ((*(uint8_t *)&(((uint8_t *)&number)[7]))) >> 7; // last bit

        char* out = buffer;
        uint8_t count = prec;
        if (prec < 10) {
            unsigned long rounding = 2; // long is 2.5x faster than double on the ESP8266
            while(count--) {
                rounding *= 10;
            }
            number += 1.0 / rounding;
        } else {
            T rounding = 2.0;
            while(count--) {
                rounding *= 10;
            }
            number += 1.0 / rounding;
        }

        uint8_t len;
        if (number <= UINT16_MAX) {
            auto value = (uint16_t)number;
            number -= value;
            len = _unsignedtoa_len(value, out, 10);
        }
        else if (number <= ULONG_MAX) {
            auto value = (unsigned long)number;
            number -= value;
            len = _unsignedtoa_len(value, out, 10);
        }
        else {
            auto value = (unsigned long long)number;
            number -= value;
            len = _unsignedtoa_len(value, out, 10);
        }
        out += len;

        size--; // NUL byte

        if (len + prec >= size) {
            prec = size - 1 - len; // reduce precision to fit into the buffer, messes up rounding
            len = size + prec; // 1 gets added later
        }
        else {
            len += prec;
        }

        if (prec) {
            len++;
            *out++ = '.';
            while(prec--) {
                number *= 10.0;
                uint8_t digit = (uint8_t)number;
                *out++ = digit + '0';
                number -= digit;
            }
            *out = 0;
        }

        return len;
    }
#endif

    void _writeNumberString(char *str, char *valPtr, printf_size_t len) {
        if (format.precision != PRECISION_NONE || format.width) {
            printf_size_t padding = 0;
            printf_size_t zero_padding = 0;
            if (len < format.precision && format.precision != PRECISION_NONE) { // precision is min. number of digits, if less, 0 padded
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
        } else {
            _writeRawString(str);
        }
    }

public:
    template <typename T, typename UT>
    void prepareLong(T value) {
        char buffer[SAFE_SIZE(UNSIGNEDTOA_MAX_STR_LEN(sizeof(value)))];
        char *ptr = buffer;
        uint8_t len;
        _addNumberFlags(ptr, value); // handles negative numbers
        if (!value && format.precision == 0) {
            *ptr = 0; // A precision of 0 means that no character is written for the value 0.
            len = 0;
        }
        else {
            // GCC does not like the T in std::make_unsigned<T>::type
            // error: need 'typename' before 'std::make_unsigned<_IntType>::type' because 'std::make_unsigned<_IntType>' is a dependent scope
            // typedef std::make_unsigned<T>::type UTT;
            // auto test_test = (UTT)value;
            len = _unsignedtoa_len((UT)value, ptr, _getRadix(), format.flagsUpperCase ? HEX_UPPERCASE : HEX_LOWECASE);
        }
        _writeNumberString(buffer, ptr, len);
    }

    template <typename T>
    void prepareDouble(T value) {
        char buffer[FLOAT_MAX_STR_LEN];
        char *ptr = buffer;
        _addNumberFlags(ptr, value); // handles negative numbers

        char fmt[UNSIGNEDTOA_MAX_STR_LEN(printf_size_t) + 5] = "%.";
        char *ptr2 = fmt + 2;
        ptr2 += _unsignedtoa_len((format.precision != PRECISION_NONE) ? format.precision : 6, ptr2, 10);
        *ptr2++ = _getFloatSpecifier();
        *ptr2 = 0;

        //// * didnt work for some reason
        //// "%.*f"


        int len = __real_snprintf(ptr, sizeof(buffer) - 2, fmt, value);
        if (len >= sizeof(buffer) - 2) {
            len = sizeof(buffer) - 3;
            ptr[len] = 0;
        }

//        uint8_t len = _dtostrf_s(value, (format.precision != PRECISION_NONE) ? format.precision : 6/*default*/, ptr, sizeof(buffer) - 2/* for _addNumberFlags() */);
//        uint8_t len = 0;

        format.precision = PRECISION_NONE; // remove precision since it has been applied already
        _writeNumberString(buffer, ptr, len);
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
            PRINTF_MEMCPY(parser.writePtr, str, len);
            parser.writePtr += len;
            parser.size -= len;
        } else {
            PRINTF_MEMCPY(parser.writePtr, str, parser.size);
            parser.writePtr += parser.size;
            parser.size = 0;
        }
        __PRINTF_VALIDATE();
    }

public:
    PrintfFormat_t format;
    PrintfParser_t parser;

#if !PRINTF_WRAPPER_ALWAYS_USE_PROGMEM
    ReadCharFunc_t getChar;
    MemCopyFunc_t memCopy;
#endif

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

    //void _addFormatFlags(char *&ptr) {
    //    if (format.flagsLeftJustify) {
    //        *ptr++ = '-';
    //    }
    //    if (format.flagsPreceedPlus) {
    //        *ptr++ = '+';
    //    }
    //    else if (format.flagsInsertSpace) {
    //        *ptr++ = ' ';
    //    }
    //    if (format.flagsZeroPadded) {
    //        *ptr++ = '0';
    //    }
    //    if (format.width) {
    //        ptr += _unsignedtoa_len(format.width, ptr, 6, 10);
    //    }
    //    if (format.precision != PRECISION_NONE) {
    //        *ptr++ = '.';
    //        ptr += _unsignedtoa_len(format.precision, ptr, 6, 10);
    //    }
    //}

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
    void _addNumberFlags(char *&ptr, T &value) {
        if (format.flagsHex || format.flagsOctal) {
            if (format.flagsHash && value != 0) {       // Used with o, x or X specifiers the value is preceeded with 0, 0x or 0X respectively for values different than zero.
                *ptr++ = '0';               // for octal and hex
                if (format.flagsHex) {      // 0x
                    *ptr++ = format.flagsUpperCase ? 'X' : 'x';
                }
            }
        }
        else {
            if (format.specifier != SPECIFIER_UINT) {
                if (value < 0) {
                    value = -value;
                    *ptr++ = '-';
                }
                else if (format.flagsPreceedPlus) {
                    *ptr++ = '+';
                }
                else if (format.flagsInsertSpace) {
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

    void __copyBuffer() {
        __validate(1);
        PRINTF_MEMCPY(orgBuffer, bufferCopy + bufferStart, bufferSize);
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

    char *orgBuffer, *bufferCopy, *bufferPtr;
    int bufferStart, bufferEnd, bufferSize, bufferCopySize;

public:
    const char *formatString;
#endif

public:
    int __snprintf(size_t size, const char *fmtStr, va_list va) {

        auto &state = parser.state;
        char currentChar;
        while ((currentChar = PRINTF_GETCHAR(fmtStr))) {

            if (state == STATE_BEGIN) {

                if (currentChar == '%') {
                    if (PRINTF_GETCHAR(fmtStr + 1) == '%') { // escaped %
                        writeChar('%');
                        fmtStr += 2;
                    }
                    else { // format string begins
                        clearFormat();
                        parser.state = STATE_FLAGS;
                        fmtStr++;
                    }
                }
                else {
                    const char *endPtr = fmtStr;
                    while(currentChar != 0 && currentChar != '%') {
                        currentChar = PRINTF_GETCHAR(++endPtr);
                    }
                    copyFormatString(fmtStr, endPtr - fmtStr);
                    fmtStr = endPtr;
                    __PRINTF_VALIDATE();
                }

            } else {
                if (state == STATE_FLAGS) {
                    switch (currentChar) {
                    case '-':
                        format.flagsLeftJustify = 1;
                        break;
                    case '+':
                        format.flagsPreceedPlus = 1;
                        break;
                    case '#':
                        format.flagsHash = 1;
                        break;
                    case ' ':
                        format.flagsInsertSpace = 1;
                        break;
                    case '0':
                        format.flagsZeroPadded = 1;
                        break;
#if PRINTF_WRAPPER_HAVE_MAC_ADDRESS
                    case 'M':
                        format.modifier = MODIFIER_MAC_ADDR;
                        break;
#endif
#if PRINTF_WRAPPER_HAVE_IPv4
                    case 'j':
                        format.modifier = MODIFIER_IPv4_ADDR;
                        break;
#endif
#if PRINTF_WRAPPER_HAVE_IPv6
                    case 'J':
                        format.modifier = MODIFIER_IPv6_ADDR;
                        break;
#endif
                    default:
                        state = STATE_WIDTH;
                        goto nextStateWidth;
                    }
                    fmtStr++;
                    continue;
                }
    nextStateWidth:
                if (state == STATE_WIDTH) {
                    if (currentChar == '*') {
                        format.width = (printf_size_t)va_arg(va, unsigned int);
                    }
                    else if (isdigit(currentChar)) {
                        format.width = (format.width * 10) + (currentChar - '0');
                        fmtStr++;
                        continue;
                    }
                    state = STATE_PRECISION;
                }
                if (state == STATE_PRECISION) {
                    if (currentChar == '.') {
                        format.precision = 0;
                        state = STATE_PRECISION_DIGITS;
                        fmtStr++;
                        continue;
                    }
                    state = STATE_LENGTH;
                }
                if (state == STATE_PRECISION_DIGITS) {
                    if (currentChar == '*') {
                        format.precision = (printf_size_t)va_arg(va, unsigned int);
                    }
                    else if (currentChar == '.') {
                        parser.state = STATE_ERROR;
                        //fmtStr++;
                        continue;
                    }
                    else if (isdigit(currentChar)) {
                        format.precision = (format.precision * 10) + (currentChar - '0');
                        fmtStr++;
                        continue;
                    }
                    state = STATE_LENGTH;
                }
                if (state == STATE_LENGTH) {
                    switch (currentChar) {
                    case 'l':
                        if (format.lengthLong) {
                            format.lengthLong = 0;
                            format.lengthLongLong = 1;
                            state = STATE_SPECIFIER;
                        }
                        else {
                            format.lengthLong = 1;
                        }
                        break;
#if PRINTF_WRAPPER_HAVE_LONG_DOUBLE
                    case 'L':
                        format.lengthLongDouble = 1;
                        state = STATE_SPECIFIER;
                        break;
#endif
#if PRINTF_WRAPPER_HAVE_SHORT_INT
                    case 'h':
                        if (format.lengthHalf) {
                            format.lengthHalf = 0;
                            format.lengthHalfHalf = 1;
                            state = STATE_SPECIFIER;
                        }
                        else {
                            format.lengthHalf = 1;
                        }
                        break;
#endif
                    default:
                        state = STATE_SPECIFIER;
                        goto nextStateSpecifier;
                    }
                    fmtStr++;
                    continue;
                }
    nextStateSpecifier:
                if (state == STATE_SPECIFIER) {
                    switch (currentChar) {
                    case 'i':
                    case 'd':
                        format.specifier = SPECIFIER_INT;
                        break;
                    case 'u':
                        format.specifier = SPECIFIER_UINT;
                        break;
                    case 'o':
                        format.specifier = SPECIFIER_OCTAL;
                        format.flagsOctal = 1;
                        break;
                    case 'x':
                        format.specifier = SPECIFIER_HEX;
                        format.flagsHex = 1;
                        break;
                    case 'X':
                        format.specifier = SPECIFIER_HEX;
                        format.flagsHex = 1;
                        format.flagsUpperCase = 1;
                        break;
                    case 'p':
                        format.specifier = SPECIFIER_POINTER;
                        break;
                    case 'f':
                        format.specifier = SPECIFIER_FLOAT;
                        break;
                    case 'a':
                        format.specifier = SPECIFIER_FLOAT;
                        format.flagsHex = 1;
                        break;
                    case 'A':
                        format.specifier = SPECIFIER_FLOAT;
                        format.flagsHex = 1;
                        format.flagsUpperCase = 1;
                        break;
                    case 'F':
                        format.specifier = SPECIFIER_FLOAT;
                        format.flagsUpperCase = 1;
                        break;
                    case 'e':
                        format.specifier = SPECIFIER_EXP;
                        break;
                    case 'E':
                        format.specifier = SPECIFIER_EXP;
                        format.flagsUpperCase = 1;
                        break;
                    case 's':
                        format.specifier = SPECIFIER_STRING;
                        break;
                    case 'c':
                        format.specifier = SPECIFIER_CHAR;
                        break;
#if PRINTF_WRAPPER_HAVE_PSTRING
                    case 'P':
                        format.specifier = SPECIFIER_PSTRING;
                        break;
#endif
#if HAVE_PRINTF_WRAPPER_BINARY
                    case 'B':
                        format.specifier = SPECIFIER_BINARY;
                        break;
#endif
#if PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT
                    case 'I':
                        format.specifier = SPECIFIER_IPADDRESS_OBJECT;
                        break;
    #   endif
#if PRINTF_WRAPPER_HAVE_STRING_OBJECT
                    case 'S':
                        format.specifier = SPECIFIER_STRING_OBJECT;
                        break;
#endif
#if PRINTF_WRAPPER_HAVE_N
                    case 'n':
                        format.specifier = SPECIFIER_N;
                        break;
#endif
                    default:
                        parser.state = STATE_ERROR;
                        //fmtStr++;
                        continue;
                    }
                    state = STATE_END; // end of format
                }
                if (state != STATE_END) {
                    fmtStr++;
                    continue;
                }
                fmtStr++;

                PrintfSpecifier_enum specifier = format.specifier;

                if (specifier <= SPECIFIER_HEX) {
                    // SPECIFIER_INT
                    // SPECIFIER_UINT
                    // SPECIFIER_OCTAL
                    // SPECIFIER_HEX

#if PRINTF_WRAPPER_HAVE_SHORT_INT
                    if (format.lengthHalfHalf) {
                        prepareLong<int8_t, uint8_t>((int8_t)va_arg(va, int));
                    }
                    else if (format.lengthHalf) {
                        prepareLong<int16_t, uint16_t>((int16_t)va_arg(va, int));
                    }
                    else
#endif
                    if (format.lengthLong) {
                        prepareLong<long, unsigned long>(va_arg(va, long));
                    }
                    else if (format.lengthLongLong) {
                        prepareLong<long long, unsigned long>(va_arg(va, long long));
                    } else {
#if UINT_MAX == ULONG_MAX
                        // GCC creates extra code even if the size of the types match
                        prepareLong<long, unsigned long>((long)va_arg(va, int));
#else
                        prepareLong<int, unsigned int>(va_arg(va, int));
#endif
                    }
                }
                else if (specifier == SPECIFIER_STRING) {
#if PRINTF_WRAPPER_ALWAYS_USE_PROGMEM
                    preparePString(va_arg(va, PGM_P));
#else
                    prepareString(va_arg(va, const char *));
#endif
                }
#if PRINTF_WRAPPER_HAVE_PSTRING
                else if (specifier == SPECIFIER_PSTRING) {
                    preparePString(va_arg(va, PGM_P));
                }
#endif
                else if (specifier == SPECIFIER_CHAR) {
                    writeChar(va_arg(va, int));
                }
                else if (specifier == SPECIFIER_POINTER) {
                    preparePointer(va_arg(va, const void *));
                }
#if PRINTF_WRAPPER_HAVE_N
                else if (specifier == SPECIFIER_N) {
                    prepareN(va_arg(va, void *));
                }
#endif
#if PRINTF_WRAPPER_HAVE_STRING_OBJECT
                else if (specifier == SPECIFIER_STRING_OBJECT) {
                    prepareString(va_arg(va, String *)->c_str());
                }
#endif
#if PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT
                else if (specifier == SPECIFIER_IPADDRESS_OBJECT) {
                    auto str = va_arg(va, IPAddress *)->toString();
                    prepareString(str.c_str());
                }
#endif
                else if (specifier == SPECIFIER_FLOAT || specifier == SPECIFIER_EXP) {
                    if (format.flagsHex) {
#if PRINTF_WRAPPER_HAVE_IPv4
                        if (format.modifier == MODIFIER_IPv4_ADDR) {
                            prepareIPAddress(va_arg(va, uint32_t));
                            continue;
                        }
#if PRINTF_WRAPPER_HAVE_MAC_ADDRESS
                        else
#endif
#endif
#if PRINTF_WRAPPER_HAVE_MAC_ADDRESS
                        if (format.modifier == MODIFIER_MAC_ADDR) {
                            prepareMacAddress(va_arg(va, const uint8_t *));
                            continue;
                        }
#if PRINTF_WRAPPER_HAVE_IPv6
                        else
#endif
#endif
#if PRINTF_WRAPPER_HAVE_IPv6
                        if (format.modifier == MODIFIER_IPv6_ADDR) {
                            prepareIPv6Address(va_arg(va, const uint8_t *));
                            continue;
                        }
#endif
                    }
#if PRINTF_WRAPPER_HAVE_LONG_DOUBLE
                    if (format.lengthLongDouble) {
                        prepareDouble(va_arg(va, long double));
                    }
                    else {
                        prepareDouble(va_arg(va, double));
                    }
#else
                    prepareDouble(va_arg(va, double));
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
            *parser.writePtr = 0;
        }

        __PRINTF_COPY_BUFFER();

        return parser.outputLength;
    }


};

#if PRINTF_WRAPPER_DEBUG
void __dump_var_vargs(const void *arg, va_list va) {
    const void *ptr = arg;
    char tmp[100];
    int n = 10;
    while(n--) {
        __real_snprintf(tmp, 100, "%p ", ptr);
        char *p = tmp;
        while(*p) {
            os_putc(*p++);
        }
        ptr = va_arg(va, const void *);
    }
    os_putc('\n');
}

void __dump_var_args(const void *arg, ...) {
    va_list va;
    va_start(va, arg);
    __dump_var_vargs(arg, va);
    va_end(va);
}
#endif

__ALWAYS_INLINE__ inline int __vsnprintf_wrapper_plus(char *buffer, size_t size PRINTF_PSTRING_FORMAT_ARG, const char *fmtStr, va_list va) {
    PrintfWrapper printf;
#if __PRINTF_DATA_INTEGRITY_CHECK
    printf.formatString = fmtStr;
    __printf = &printf;
#endif
#if PRINTF_WRAPPER_ALWAYS_USE_PROGMEM
    printf.setup((printf_size_t)size, buffer);
#else
    printf.setup((printf_size_t)size, buffer, formatIsPString);
#endif
    printf.__snprintf(size, fmtStr, va);
    return printf.parser.outputLength;
}

int __wrap_snprintf(char *buffer, size_t size, const char *format, ...) {
    int result;
    va_list arg;
    va_start(arg, format);
    result = __vsnprintf_wrapper_plus(buffer, size, PRINTF_PSTRING_FORMAT_CSTR(format), arg);
    va_end(arg);
    return result;
}

int __wrap_snprintf_P(char *buffer, size_t size, PGM_P format, ...) {
    int result;
    va_list arg;
    va_start(arg, format);
    result = __vsnprintf_wrapper_plus(buffer, size, PRINTF_PSTRING_FORMAT_PSTR(format), arg);
    va_end(arg);
    return result;
}

int __wrap_vsnprintf(char *buffer, size_t size, const char *format, va_list va) {
    return __vsnprintf_wrapper_plus(buffer, size, PRINTF_PSTRING_FORMAT_CSTR(format), va);
}

int __wrap_vsnprintf_P(char *buffer, size_t size, PGM_P format, va_list va) {
    return __vsnprintf_wrapper_plus(buffer, size, PRINTF_PSTRING_FORMAT_PSTR(format), va);
}

static void _test_printf(int (*func)(char *, size_t, PGM_P, va_list), char *buf, size_t size, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    func(buf, size, fmt, va);
    va_end(va);
}

void sprintf_wrapper_info(Print &print) {
    PGM_P test = PSTR("!#");
    PGM_P result[2] = { "no", "yes" };
    char buf[4];
    snprintf(buf, sizeof(buf), "%P", test);
    auto snprintf_wrapped = strcmp_P(buf, test) == 0;
    _test_printf(vsnprintf, buf, sizeof(buf), "%P", test);
    auto vsnprintf_wrapped = strcmp_P(buf, test) == 0;
    snprintf_P(buf, sizeof(buf), PSTR("%P"), test);
    auto snprintf_P_wrapped = strcmp_P(buf, test) == 0;
    _test_printf(vsnprintf_P, buf, sizeof(buf), "%P", test);
    auto vsnprintf_P_wrapped = strcmp_P(buf, test) == 0;
    print.printf_P(PSTR("snprintf wrapped: %s\nsnprintf_P wrapped: %s\n"), result[snprintf_wrapped], result[snprintf_P_wrapped]);
    print.printf_P(PSTR("vsnprintf wrapped: %s\nvsnprintf_P wrapped: %s\n"), result[vsnprintf_wrapped], result[vsnprintf_P_wrapped]);
    print.printf_P(PSTR("%%s supports PGM_P: %s\n"), result[PRINTF_WRAPPER_ALWAYS_USE_PROGMEM]);

}
