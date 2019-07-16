## sprintf replacement for ESP8266

The sprintf replacement offers vsnprintf, snprintf, vsnprintf_P and snprintf_P with improved performance and functionality compared to the version that comes with the esp8266-nonos-sdk framework.

Originally it was just a wrapper for snprintf with added functionality, but the performace was pretty bad. It turned out that snprintf of the espressif framework is already pretty slow and most snprintf calls have been replaced.

It should be possible to compile for ESP32 and other platforms.

## Format

%[flags or modifier][width][.precision][length]specifier

## Additional specifiers

### PROGMEM String

Specifier: P
Parameter: PGM_P or __FlashStringHelper *

snprintf_wrapper(.. , .., "Print from %P directly", PSTR("PROGMEM"));

snprintf_wrapper_P(.. , .., PSTR("Print from %P directly with PROGMEM format"), PSTR("PROGMEM"));

### String Object

Specifier: S
Parameter: String *

snprintf_wrapper(.. , .., "Print %S objects directly", &String("String"));

### IPAddress Object

Specifier: I
Parameter: IPAddress *

snprintf_wrapper(.. , .., "Print IPAddress objects: %I", &IPAddress(1, 2, 3, 4));

## Modifiers

The modifiers use the specifier a and A, which is originally float, but A seems to be a much better fit for addresses. a and A without the modifiers are still working

### IPv4 Address

Specifier: a or A
Modifier: j

snprintf_wrapper(.. , .., "IP: %jA", (uint32_t)0x04040808);

Output: "8.8.4.4"

### IPv6 Address

Specifier: a for lowercase, A for uppercase
Modifier: J
Parameter: const uint8_t *, const uint16_t *, 16 byte
Flags: 0 for leading zeros

uint16_t addr[8] = { 0x120, 0xb80d, 0, 0, 0, 0xff, 0x4200, 0x2839 };
snprintf_wrapper(.. , .., "%Ja", addr);

Output:

%0Ja        "2001:0db8:0000:0000:0000:ff00:0042:3928"
%0jA        "2001:0DB8:0000:0000:0000:FF00:0042:3928"
%Ja         "2001:db8::ff00:42:3928"
%Ja         "::1"

### MAC Address

Specifier: a for lowercase, A for uppercase
Modifier: M
Parameter: const uint8_t, 6 byte
Flags: # use '-' as separator. default is ':', width 2 will not print the separator

uint8_t mac[6] = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc };
snprintf_wrapper(.. , .., "%Ma", mac);

Output:

%Ma     "12:34:56:78:9a:bc"
%#MA    "12-34-56-78-9A-BC"
%M2A    "123456789ABC"
