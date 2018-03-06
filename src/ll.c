#include "ll.h"

#include <limits.h>
#include <intrin.h>

uint32_t uint32_bsr(uint32_t x)
{
    unsigned long res;
    return _BitScanReverse(&res, (unsigned long) x) ? res : UINT32_MAX;
}

uint32_t uint32_bsf(uint32_t x)
{
    unsigned long res;
    return _BitScanForward(&res, (unsigned long) x) ? res : UINT32_MAX;
}

uint8_t uint8_bsr(uint8_t x)
{
    return (uint8_t) uint32_bsr((uint8_t) x);
}

uint8_t uint8_bsf(uint8_t x)
{
    return (uint8_t) uint32_bsf((uint8_t) x);
}

#ifdef _M_X64

size_t size_mul(size_t *p_hi, size_t a, size_t b)
{
    unsigned __int64 hi;
    size_t res = (size_t) _umul128((unsigned __int64) a, (unsigned __int64) b, &hi);
    *p_hi = (size_t) hi;
    return res;
}

size_t size_add(size_t *p_car, size_t x, size_t y)
{
    unsigned __int64 res;
    *p_car = _addcarry_u64(0, x, y, &res);
    return res;
}

size_t size_sub(size_t *p_bor, size_t x, size_t y)
{
    unsigned __int64 res;
    *p_bor = _subborrow_u64(0, x, y, &res);
    return res;
}

size_t size_sum(size_t *p_hi, size_t *args, size_t args_cnt)
{
    unsigned __int64 lo = 0, hi = 0;
    for (size_t i = 0; i < args_cnt; _addcarry_u64(_addcarry_u64(0, lo, args[i++], &lo), hi, 0, &hi));
    *p_hi = (size_t) hi;
    return (size_t) lo;
}

size_t size_bsr(size_t x)
{
    unsigned long res;
    return _BitScanReverse64(&res, (unsigned __int64) x) ? res : SIZE_MAX;
}

size_t size_bsf(size_t x)
{
    unsigned long res;
    return _BitScanForward64(&res, (unsigned __int64) x) ? res : SIZE_MAX;
}

#elif defined _M_IX86

size_t size_mul(size_t *p_hi, size_t x, size_t y)
{
    union { unsigned long long val; struct { size_t lo, hi; }; } res = { .val = (unsigned long long) x * (unsigned long long) y };
    *p_hi = res.hi;
    return res.lo;
}

size_t size_add(size_t *p_car, size_t x, size_t y)
{
    union { unsigned long long val; struct { size_t lo, hi; }; } res = { .val = (unsigned long long) x + (unsigned long long) y } ;
    *p_car = res.hi;
    return res.lo;
}

size_t size_sub(size_t *p_bor, size_t x, size_t y)
{
    union { unsigned long long val; struct { size_t lo, hi; }; } res = { .val = (unsigned long long) x - (unsigned long long) y };
    *p_bor = 0 - res.hi;
    return res.lo;
}

size_t size_sum(size_t *p_hi, size_t *args, size_t args_cnt)
{
    union { unsigned long long val; struct { size_t lo, hi; }; } res = { .val = 0 };
    for (size_t i = 0; i < args_cnt; res.val += args[i++]);
    *p_hi = res.hi;
    return res.lo;
}

size_t size_bsr(size_t x)
{
    return (size_t) uint32_bsr((uint32_t) x);
}

size_t size_bsf(size_t x)
{
    return (size_t) uint32_bsf((uint32_t) x);
}

#endif

bool bit_test(void *Arr, size_t bit)
{
    uint8_t *arr = Arr;
    return !!(arr[bit >> 3] & (1 << (bit & 7)));
}

void bit_set(void *Arr, size_t bit)
{
    uint8_t *arr = Arr;
    arr[bit >> 3] |= 1 << (bit & 7);
}

void bit_reset(void *Arr, size_t bit)
{
    uint8_t *arr = Arr;
    arr[bit >> 3] &= ~(1 << (bit & 7));
}
