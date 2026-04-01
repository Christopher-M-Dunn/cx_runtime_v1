#include <stdint.h>
#include "../../include/ci.h"
#include "addsub_common.h"

#ifndef ADDSUB_H
#define ADDSUB_H

__CX__ static inline int32_t add( int32_t a, int32_t b ) 
{
    return CX_REG_HELPER(0, a, b);
}

__CX__ static inline int32_t sub( int32_t a, int32_t b )
{
    return CX_REG_HELPER(2, a, b);
}

__CX__ static inline int32_t add_1000( int32_t a, int32_t b )
{
    return CX_REG_HELPER(4, a, b);
}

#endif