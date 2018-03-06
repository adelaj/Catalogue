#pragma once

///////////////////////////////////////////////////////////////////////////////
//
//  Functions providing low level facilities
//

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

size_t size_add(size_t *, size_t, size_t);
size_t size_sub(size_t *, size_t, size_t);
size_t size_sum(size_t *, size_t *, size_t);
size_t size_mul(size_t *, size_t, size_t);
uint8_t uint8_bsr(uint8_t);
uint8_t uint8_bsf(uint8_t);
uint32_t uint32_bsr(uint32_t);
uint32_t uint32_bsf(uint32_t);
size_t size_bsr(size_t);
size_t size_bsf(size_t);

#define BYTE_CNT(BIT) (((BIT) + 7) >> 3)
bool bit_test(void *, size_t);
void bit_set(void *, size_t);
void bit_reset(void *, size_t);