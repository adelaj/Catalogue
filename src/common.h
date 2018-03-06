#pragma once

#define _CRT_SECURE_NO_WARNINGS

// Suppressing some MSVS warnings
#pragma warning(disable : 4116) // "Unnamed type definition in parentheses"
#pragma warning(disable : 4200) // "Zero-sized array in structure/union"
#pragma warning(disable : 4201) // "Nameless structure/union"
#pragma warning(disable : 4204) // "Non-constant aggregate initializer"
#pragma warning(disable : 4221) // "Initialization by using the address of automatic variable"
#define restrict __restrict
#define inline __inline
#define alignof __alignof
#define _Static_assert static_assert

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define sizeof_field(type, member) (sizeof(((type *) NULL)->member))
#define strlenof(string) (_countof(string) - 1)

#define MAX(a, b) \
    ((a) >= (b) ? (a) : (b))
#define MIN(a, b) \
    ((a) <= (b) ? (a) : (b))
#define DIST(a, b) \
    ((a) >= (b) ? (a) - (b) : (b) - (a))
#define ABS(a) \
    ((a) >= 0 ? (a) : -(a))
#define CLAMP(x, a, b) \
    ((x) >= (a) ? (x) <= (b) ? (x) : (b) : (a))
#define SIGN(x, y) \
    ((x) > (y) ? 1 : (x) == (y) ? 0 : -1)

// Convert value (which is often represented by a macro) to string literal
#define TOSTRING_EXPAND(Z) #Z
#define TOSTRING(Z) TOSTRING_EXPAND(Z)

#define TEMP_BUFF 2048
#define SMALL_BUFF (UCHAR_MAX + 1)
#define LARGE_BUFF (USHRT_MAX + 1)
