#include "ll.h"
#include "memory.h"

#include <stdlib.h>
#include <limits.h>
#include <errno.h>

void *malloc2(size_t cap, size_t sz, size_t diff, bool clr)
{
    size_t hi, pr = size_mul(&hi, cap, sz);
    if (!hi)
    {
        size_t car, cap2 = size_add(&car, pr, diff);
        if (!car) return clr ? calloc(cap2, 1) : malloc(cap2);
    }
    errno = ERANGE;
    return NULL;
}

void *malloc3(size_t *p_cap, size_t sz, size_t diff, bool clr)
{
    size_t cap = *p_cap, log2 = size_bsr(cap);
    if ((size_t) ~log2)
    {
        size_t cap2 = (size_t) 1 << log2;
        if ((cap == cap2) || (cap2 <<= 1))
        {
            void *tmp = malloc2(cap2, sz, diff, clr);
            *p_cap = tmp ? cap2 : 0;
            return tmp;
        }
        errno = ERANGE;
    }
    return NULL;
}

void *realloc2(void *src, size_t cap, size_t sz, size_t diff)
{
    size_t hi, pr = size_mul(&hi, cap, sz);
    if (!hi)
    {
        size_t car, cap2 = size_add(&car, pr, diff);
        if (!car) return realloc(src, cap2);
    }
    errno = ERANGE;
    return NULL;
}

void *realloc3(void *src, size_t *p_cap, size_t sz, size_t diff)
{
    size_t cap = *p_cap, log2 = size_bsr(cap);
    if ((size_t) ~log2)
    {
        size_t cap2 = (size_t) 1 << log2;
        if ((cap == cap2) || (cap2 <<= 1))
        {
            void *tmp = realloc2(src, cap2, sz, diff);
            *p_cap = tmp ? cap2 : 0;
            return tmp;
        }
        errno = ERANGE;
    }
    return NULL;
}

bool array_init_strict(void *P_arr, size_t cap, size_t sz, size_t diff, bool clr)
{
    void **restrict p_arr = P_arr;
    return (*p_arr = malloc2(cap, sz, diff, clr)) != NULL || !((cap && sz) || diff);
}

bool array_init(void *P_arr, size_t *p_cap, size_t sz, size_t diff, bool clr)
{
    void **restrict p_arr = P_arr;
    return (*p_arr = malloc3(p_cap, sz, diff, clr)) != NULL || !((*p_cap && sz) || diff);
}

bool array_resize_strict(void *P_arr, size_t cap, size_t sz, size_t diff, enum array_resize_mode mode, size_t *args, size_t args_cnt)
{
    void **restrict p_arr = P_arr;
    size_t car, cnt = size_sum(&car, args, args_cnt);
    if (!car)
    {
        if (((mode & ARRAY_RESIZE_EXTEND_ONLY) && (cnt <= cap)) || ((mode & ARRAY_RESIZE_REDUCE_ONLY) && (cnt >= cap))) return 1;
        void *tmp = realloc2(*p_arr, cnt, sz, diff);
        if (tmp || !((cnt && sz) || diff))
        {
            *p_arr = tmp;
            return 1;
        }
        return 0;
    }
    errno = ERANGE;
    return 0;
}

bool array_resize(void *P_arr, size_t *p_cap, size_t sz, size_t diff, enum array_resize_mode mode, size_t *args, size_t args_cnt)
{
    void **restrict p_arr = P_arr;
    size_t car, cnt = size_sum(&car, args, args_cnt);
    if (!car)
    {
        if (((mode & ARRAY_RESIZE_EXTEND_ONLY) && (cnt <= *p_cap)) || ((mode & ARRAY_RESIZE_REDUCE_ONLY) && (cnt >= *p_cap))) return 1;
        void *tmp = realloc3(*p_arr, &cnt, sz, diff);
        if (tmp || !((cnt && sz) || diff))
        {
            *p_arr = tmp;
            *p_cap = cnt;
            return 1;
        }
        return 0;
    }
    errno = ERANGE;
    return 0;
}
