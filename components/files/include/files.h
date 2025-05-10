#pragma once

#define str(s) #s
#define EMBEDDED_FILE(filename) extern const uint8_t filename##_start[] asm(str(_binary_##filename##_start));    \
                                extern const uint8_t filename##_end[] asm(str(_binary_##filename##_end));
