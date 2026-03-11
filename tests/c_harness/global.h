#ifndef TEST_GLOBAL_H
#define TEST_GLOBAL_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef u8 bool8;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef ARRAY_COUNT
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#endif

#endif
