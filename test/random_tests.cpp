
#include <string.h>
#include <assert.h>
#include "random_tests.h"
#include "printf_wrapper.h"

#if _WIN32
#include <Arduino_compat.h>
#else
#include <Arduino.h>
#include <ESP8266WiFi.h>
#endif

#define __FILL(ptr, len) memset(ptr, 0xcc, len)
#define __ASSERT(expr) assert(expr);

void run_tests(snprintf_func_t func, bool extra) {
    char buf[1024];

    if (extra) {
        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "a%%a") == 3);
        __ASSERT(!strcmp(buf, "a%a")); 

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "a~") == 2);
        __ASSERT(!strcmp(buf, "a~"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%d", 100) == 3);
        __ASSERT(!strcmp(buf, "100"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%lu", 123UL) == 3);
        __ASSERT(!strcmp(buf, "123"));

#if PRINTF_WRAPPER_HAVE_SHORT_INT
        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%hhu", (uint8_t)-1) == 3);
        __ASSERT(!strcmp(buf, "255"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%hhu", 0xffffffff) == 3);
        __ASSERT(!strcmp(buf, "255"));
#endif

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%s", "test") == 4);
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 20, "%js", 0x04040808) == 7);
        __ASSERT(!strcmp(buf, "8.8.4.4"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%s", String("test").c_str()) == 4);
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%s", IPAddress(1, 2, 3, 4).toString().c_str()) == 7);
        __ASSERT(!strcmp(buf, "1.2.3.4"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%Ss", &String("test")) == 4);
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%Is", &IPAddress(4, 3, 2, 1)) == 7);
        __ASSERT(!strcmp(buf, "4.3.2.1"));

#if PRINTF_WRAPPER_HAVE_PSTRING
        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, 10, "%P", PSTR("test")) == 4);
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%2.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "tes"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%2.5P", PSTR("test"));
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%0-10.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "tes       "));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%-10.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "tes       "));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%010.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "0000000tes"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%3.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "tes"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%10.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "       tes"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%3.10P", PSTR("test"));
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%-3.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "tes"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%-3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%-3.10P", PSTR("test"));
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%+3.10P", PSTR("test"));
        __ASSERT(!strcmp(buf, "test"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%+10.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "       tes"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%+-10.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "tes       "));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "% +-10.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "tes       "));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%0+-10.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "tes       "));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%000010.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "0000000tes"));

        __FILL(buf, sizeof(buf));
        func(buf, sizeof(buf), "%0+10.3P", PSTR("test"));
        __ASSERT(!strcmp(buf, "0000000tes"));

        for(int i = 1; i < 16; i++) {
            char str[17];
            strcpy(str, "0000000tes");
            str[i - 1] = 0;
            printf("loop %d\n", i);
            __FILL(buf, sizeof(buf));
            __ASSERT(func(buf, i, "%0+10.3P", PSTR("test")) == 10);
            __ASSERT(!strcmp(buf, str));
        }

        for(int i = 1; i < 16; i++) {
            char str[17];
            strcpy(str, "tes       ");
            str[i - 1] = 0;
            __FILL(buf, sizeof(buf));
            printf("loop %d\n", i);
            __ASSERT(func(buf, i, "%-10.3P", PSTR("test")) == 10);
            __ASSERT(!strcmp(buf, str));
        }

        for(int i = 1; i < 16; i++) {
            char str[17];
            strcpy(str, "test");
            str[i - 1] = 0;
            __FILL(buf, sizeof(buf));
            printf("loop %d\n", i);
            __ASSERT(func(buf, i, "%P", PSTR("test")) == 4);
            __ASSERT(!strcmp(buf, str));
        }

        for(int i = 1; i < 16; i++) {
            char str[17];
            strcpy(str, "te   x   te");
            str[i - 1] = 0;
            __FILL(buf, sizeof(buf));
            printf("loop %d\n", i);
            __ASSERT(func(buf, i, "%-5.2Px%5.2P", PSTR("test"), PSTR("test")) == 11);
            __ASSERT(!strcmp(buf, str));
        }

        for(int i = 0; i < 1000; i++) {
            char str[100];
            char fmt[100];
            int res;
            snprintf(fmt, sizeof(fmt), "%%%d.%ds XxX %%.%ds XxX %%%ds", rand() % 20, rand() % 20, rand() % 20, rand() % 20);
            __FILL(buf, sizeof(buf));
            const char *str1 = "test string";
            const char *str2 = "test string";
            const char *str3 = "test string";
            res = snprintf(str, sizeof(str), fmt, str1, str2, str3);
            printf("loop %d\n", i);
            __ASSERT(func(buf, sizeof(buf), fmt, str1, str2, str3) == res); 
            __ASSERT(!strcmp(buf, str));
        }

        __FILL(buf, sizeof(buf));
        strcpy(buf, "xxxx");
        __ASSERT(func(buf, 0, "a") == 1);
        __ASSERT(!strcmp(buf, "xxxx"));   // size is 0, string must not be modified

#if PRINTF_WRAPPER_HAVE_IPv6
        uint16_t ipv6[8] = { 0x120, 0xb80d, 0, 0, 0, 0xff, 0x4200, 0x2839 };
        uint16_t ipv62[8] = { 0, 0, 0, 0, 0, 0, 0, 0x100 };

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%0Ja", &ipv6) == 39);
        __ASSERT(!strcmp(buf, "2001:0db8:0000:0000:0000:ff00:0042:3928"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%0JA", &ipv6) == 39);
        __ASSERT(!strcmp(buf, "2001:0DB8:0000:0000:0000:FF00:0042:3928"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%JA", &ipv6) == 22);
        __ASSERT(!strcmp(buf, "2001:DB8::FF00:42:3928"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%Ja", &ipv6) == 22);
        __ASSERT(!strcmp(buf, "2001:db8::ff00:42:3928"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%JA", &ipv62) == 3);
        __ASSERT(!strcmp(buf, "::1"));
#endif

#if PRINTF_WRAPPER_HAVE_IPv4
        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%ja", 0x04040808) == 7);
        __ASSERT(!strcmp(buf, "8.8.4.4"));
#endif

#if PRINTF_WRAPPER_HAVE_MAC_ADDRESS
        uint8_t mac[6] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc };

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%Ma", mac) == 17);
        __ASSERT(!strcmp(buf, "12:34:56:78:9a:bc"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%#MA", mac) == 17);
        __ASSERT(!strcmp(buf, "12-34-56-78-9A-BC"));

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%M2a", mac) == 12);
        __ASSERT(!strcmp(buf, "123456789abc"));
#endif

#if PRINTF_WRAPPER_HAVE_STRING_OBJECT

        String stringObject = "test test";

        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%S", &stringObject) == 9);
        __ASSERT(!strcmp(buf, "test test"));

#endif

#if PRINTF_WRAPPER_HAVE_IPADDRESS_OBJECT

        IPAddress ipObject(1, 2, 3, 4);
        __FILL(buf, sizeof(buf));
        __ASSERT(func(buf, sizeof(buf), "%I", &ipObject) == 7);
        __ASSERT(!strcmp(buf, "1.2.3.4"));

#endif

#endif

    }

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%2.3s", "test");
    __ASSERT(!strcmp(buf, "tes"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%2.5s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0-10.3s", "test");     // padding with space instead of zero if aligned to the left
    __ASSERT(!strcmp(buf, "tes       "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%-10.3s", "test");
    __ASSERT(!strcmp(buf, "tes       "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%010.3s", "test");     // zero padding
    __ASSERT(!strcmp(buf, "0000000tes"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%3.3s", "test");
    __ASSERT(!strcmp(buf, "tes"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%3s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%10.3s", "test");
    __ASSERT(!strcmp(buf, "       tes"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%3.10s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%-3.3s", "test");
    __ASSERT(!strcmp(buf, "tes"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%-3s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%-3.10s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+3.10s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+10.3s", "test");
    __ASSERT(!strcmp(buf, "       tes"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+-10.3s", "test");
    __ASSERT(!strcmp(buf, "tes       "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "% +-10.3s", "test");
    __ASSERT(!strcmp(buf, "tes       "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0+-10.3s", "test");
    __ASSERT(!strcmp(buf, "tes       "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%000010.3s", "test");
    __ASSERT(!strcmp(buf, "0000000tes"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0+10.3s", "test");     // + ignored
    __ASSERT(!strcmp(buf, "0000000tes"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%-s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%#s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+-s", "test");
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "a%~", "test"); // skips the %
    __ASSERT(!strcmp(buf, "a~"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%#+----s", "test"); // it ignores 
    __ASSERT(!strcmp(buf, "test"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "aa%j~", 10); // skips %j
    __ASSERT(!strcmp(buf, "aa~"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "aa%jys~", 10); // does not ignore y but skips %j
    __ASSERT(!strcmp(buf, "aays~"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%-d", 10);
    __ASSERT(!strcmp(buf, "10"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+d", 10);
    __ASSERT(!strcmp(buf, "+10"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+-d", 10);
    __ASSERT(!strcmp(buf, "+10"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%10.5d", 10);
    __ASSERT(!strcmp(buf, "     00010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%10.-5d", 10);      // flag after dot is invalid
    __ASSERT(!strcmp(buf, "-5d"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%10.5-d", 10);      // flag after precision is invalid
    __ASSERT(!strcmp(buf, "-d"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%10-.5d", 10);      // flag after width is invalid
    __ASSERT(!strcmp(buf, "-.5d"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%.05d", 10);      // leading zero is allowed
    __ASSERT(!strcmp(buf, "00010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%.010d", 10);      // leading zero is allowed and still treated as decimal not octal
    __ASSERT(!strcmp(buf, "0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%.000010d", 10);      // leading zeros are ignored
    __ASSERT(!strcmp(buf, "0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%00.0010d", 10);    // multiple zeros allowed
    __ASSERT(!strcmp(buf, "0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "% 00.0010d", 10);
    __ASSERT(!strcmp(buf, " 0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "% 00.0010u", 10);
    __ASSERT(!strcmp(buf, "0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0 .0010d", 10);
    __ASSERT(!strcmp(buf, " 0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%   00.0010d", 10); // single space
    __ASSERT(!strcmp(buf, " 0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+   00.0010d", 10);    // plus and space, space ignored
    __ASSERT(!strcmp(buf, "+0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "% +   00.0010d", 10);    // plus before space, space ignored
    __ASSERT(!strcmp(buf, "+0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0+   00.0010d", 10);    // plus before zero, zero ignored
    __ASSERT(!strcmp(buf, "+0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "% +5d", 10);
    __ASSERT(!strcmp(buf, "  +10"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0+5d", 10);
    __ASSERT(!strcmp(buf, "+0010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0+-5d", 10);   // alignment before padding
    __ASSERT(!strcmp(buf, "+10  "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%-0+5d", 10); // alignment before padding
    __ASSERT(!strcmp(buf, "+10  "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+05d", 10);
    __ASSERT(!strcmp(buf, "+0010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+- 5d", 10);
    __ASSERT(!strcmp(buf, "+10  "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%+ 5d", 10);
    __ASSERT(!strcmp(buf, "  +10"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "% .10d", 10);      // single leading space added
    __ASSERT(!strcmp(buf, " 0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%  .10d", 10);      // 2xspace flag, only single leading space added
    __ASSERT(!strcmp(buf, " 0000000010"));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%-10.5d", 10);
    __ASSERT(!strcmp(buf, "00010     "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%-+10.5d", 10);
    __ASSERT(!strcmp(buf, "+00010    "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0-+10.5d", 10); // alignment before padding
    __ASSERT(!strcmp(buf, "+00010    "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0+-10.5d", 10); // alignment before padding no matter where + is
    __ASSERT(!strcmp(buf, "+00010    "));

    __FILL(buf, sizeof(buf));
    func(buf, sizeof(buf), "%0+-10..5d", 10); // 2x dot is invalid
    __ASSERT(!strcmp(buf, ".5d"));

    for(int i = 1; i < 40; i++) {
        char str[40];
        strcpy(str, "test string test string test string");
        str[i - 1] = 0;
        __FILL(buf, sizeof(buf));
        const char *str1 = "test string";
        const char *str2 = "test string";
        const char *str3 = "test string";
        printf("loop %d\n", i);
        __ASSERT(func(buf, i, "%s %s %s", str1, str2, str3) == 35); 
        __ASSERT(!strcmp(buf, str));
    }

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%hhd %d", 200, 300) == 7);
    __ASSERT(!strcmp(buf, "-56 300"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%hhu", 200) == 3);
    __ASSERT(!strcmp(buf, "200"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%hu", 0xf0a1) == 5);
    __ASSERT(!strcmp(buf, "61601"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%hd", 0xf0a1) == 5);
    __ASSERT(!strcmp(buf, "-3935"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%p", 0xffffff) == 8);
    __ASSERT(!strcmp(buf, "00FFFFFF"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.d", 0) == 0);
    __ASSERT(!strcmp(buf, ""));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%5.d", 0) == 5);
    __ASSERT(!strcmp(buf, "     "));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%+.5d", 100) == 6);
    __ASSERT(!strcmp(buf, "+00100"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%+.1d", 100) == 4);
    __ASSERT(!strcmp(buf, "+100"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%+.0d", 1) == 2);
    __ASSERT(!strcmp(buf, "+1"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%+.d", 1) == 2);
    __ASSERT(!strcmp(buf, "+1"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%+.0d", 0) == 1);
    __ASSERT(!strcmp(buf, "+"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%+.d", 0) == 1);
    __ASSERT(!strcmp(buf, "+"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.1d", 100) == 3);
    __ASSERT(!strcmp(buf, "100"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.0d", 1) == 1);
    __ASSERT(!strcmp(buf, "1"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.d", 1) == 1);
    __ASSERT(!strcmp(buf, "1"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.0d", 100) == 3);
    __ASSERT(!strcmp(buf, "100"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.d", 100) == 3);
    __ASSERT(!strcmp(buf, "100"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%20.5f", 1.12345678901234) == 20);
    __ASSERT(!strcmp(buf, "             1.12346"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%20.5f", -1.12345678901234) == 20);
    __ASSERT(!strcmp(buf, "            -1.12346"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%-20.5f", -1.12345678901234) == 20);
    __ASSERT(!strcmp(buf, "-1.12346            "));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%06.2f", -12.12345678901234) == 6);
    __ASSERT(!strcmp(buf, "-12.12"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%07.2f", -12.12345678901234) == 7);
    __ASSERT(!strcmp(buf, "-012.12"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%08.2f", -12.12345678901234) == 8);
    __ASSERT(!strcmp(buf, "-0012.12"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%+08.2f", 12.12345678901234) == 8);
    __ASSERT(!strcmp(buf, "+0012.12"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%-+08.2f", 12.12345678901234) == 8);
    __ASSERT(!strcmp(buf, "+12.12  "));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%-+08.2f", -12.12345678901234) == 8);
    __ASSERT(!strcmp(buf, "-12.12  "));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.9f", 1.12345678901234) == 11);
    __ASSERT(!strcmp(buf, "1.123456789"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.11f", 1.12345678901234) == 13);
    __ASSERT(!strcmp(buf, "1.12345678901"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%05.2f", 100.77213) == 6);
    __ASSERT(!strcmp(buf, "100.77"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%5.2f", 100.77213) == 6);
    __ASSERT(!strcmp(buf, "100.77"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%-5.2f", 100.77213) == 6);
    __ASSERT(!strcmp(buf, "100.77"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.f", 100.77213) == 3);
    __ASSERT(!strcmp(buf, "101"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%05.f", 100.77213) == 5);
    __ASSERT(!strcmp(buf, "00101"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%-05.f", 100.77213) == 5);
    __ASSERT(!strcmp(buf, "101  "));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%+05.f", 100.77213) == 5);
    __ASSERT(!strcmp(buf, "+0101"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%-+05.f", 100.77213) == 5);
    __ASSERT(!strcmp(buf, "+101 "));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%- 05.f", 100.77213) == 5);
    __ASSERT(!strcmp(buf, " 101 "));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%f", 123456789012345689.0) == 25);
    __ASSERT(!strcmp(buf, "123456789012345696.000000"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%f", 123456789012345.6789) == 22);
    __ASSERT(!strcmp(buf, "123456789012345.671875"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.10f", 123456789012345.6789) == 26);
    __ASSERT(!strcmp(buf, "123456789012345.6718750000"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.10f", 123456789012.3456789) == 23);
    __ASSERT(!strcmp(buf, "123456789012.3456726074"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%f", -123456789012345.6789) == 23);
    __ASSERT(!strcmp(buf, "-123456789012345.671875"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%Lf", 123456789012345689.0) == 25);
    __ASSERT(!strcmp(buf, "123456789012345696.000000"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%#x", 100) == 4);
    __ASSERT(!strcmp(buf, "0x64"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%#x", 0) == 1);
    __ASSERT(!strcmp(buf, "0"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%x", 0x1a34) == 4);
    __ASSERT(!strcmp(buf, "1a34"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%X", 0x1a34) == 4);
    __ASSERT(!strcmp(buf, "1A34"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%o", 0775) == 3);
    __ASSERT(!strcmp(buf, "775"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%#o", 0775) == 4);
    __ASSERT(!strcmp(buf, "0775"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%#o", 0) == 1);
    __ASSERT(!strcmp(buf, "0"));

    double zero = 0.0;

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%f", 1.0 / zero) == 3);
    __ASSERT(!strcmp(buf, "inf"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%f", 1.0 / -zero) == 4);
    __ASSERT(!strcmp(buf, "-inf"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.2f", 1.0 / zero) == 3);
    __ASSERT(!strcmp(buf, "inf"));

    __FILL(buf, sizeof(buf));
    __ASSERT(func(buf, sizeof(buf), "%.2f", 1.0 / -zero) == 4);
    __ASSERT(!strcmp(buf, "-inf"));
}
