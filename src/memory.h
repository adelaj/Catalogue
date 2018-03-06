#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Much safer facilities for a memory management
//

#include "common.h"

void *malloc2(size_t, size_t, size_t, bool);
void *malloc3(size_t *, size_t, size_t, bool);
void *realloc2(void *, size_t, size_t, size_t);
void *realloc3(void *, size_t *, size_t, size_t);

// Arrays initialization procedures
bool array_init_strict(void *, size_t, size_t, size_t, bool);
bool array_init(void *, size_t *, size_t, size_t, bool);

enum array_resize_mode {
    ARRAY_RESIZE_ANY = 0,
    ARRAY_RESIZE_EXTEND_ONLY = 1, // Extension only 
    ARRAY_RESIZE_REDUCE_ONLY = 2, // Reduction only
};

// Arrays extension (5th arg5 = 0) and reduction (5th arg = 1) procedures (also can be used for a memory allocation)
bool array_resize_strict(void *, size_t, size_t, size_t, enum array_resize_mode, size_t *, size_t);
bool array_resize(void *, size_t *, size_t, size_t, enum array_resize_mode, size_t *, size_t);

// Helper macros evaluating and inserting the count of arguments
#define ARGS(...) \
    ((size_t []) { __VA_ARGS__ }), _countof(((size_t []) { __VA_ARGS__ }))
