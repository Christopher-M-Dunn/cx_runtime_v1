#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/p-ext/p-ext.h"

#define CX_SEL_TABLE_NUM_ENTRIES 1024

#define i8 int8_t
#define u8 uint8_t

#define i16 int16_t
#define u16 uint16_t

#define i32 int32_t
#define u32 uint32_t

u32 pack_32_i8(i8 a, i8 b, i8 c, i8 d) {
    u32 res = (a & 0xFF);
    res |= (b << 8) & 0xFF00;
    res |= (c << 16) & 0xFF0000;
    res |= (d << 24) & 0xFF000000;
    return res;
}

u32 pack_32_i16(i16 a, i16 b) {
    u32 res = (a & 0xFFFF);
    res |= (b << 16) & 0xFFFF0000;
    return res;
}

__CX__ void pack_and_assert_add16(i16 a, i16 b, i16 c, i16 d) {
    u32 packed_i16_0 = pack_32_i16(a, b);
    u32 packed_i16_1 = pack_32_i16(c, d);
    u32 result = add16(packed_i16_0, packed_i16_1);
    assert( (i16)GET_BITS(result, 0, 16) == (a + c) );
    assert( (i16)GET_BITS(result, 16, 16) == (b + d) );
}

__CX__ void pack_and_assert_smul16(i16 a, i16 b, i16 c, i16 d) {
    u32 packed_i16_0 = pack_32_i16(a, b);
    u32 packed_i16_1 = pack_32_i16(c, d);
    u32 result = smul16(packed_i16_0, packed_i16_1);
    assert( (i16)GET_BITS(result, 0, 16) == (a * c) );
    assert( (i16)GET_BITS(result, 16, 16) == (b * d) );
}

__CX__ void pack_and_assert_sra16(i16 a, i16 b, i16 sa) {
    u32 packed_i16_0 = pack_32_i16(a, b);
    u32 result = sra16(packed_i16_0, sa);
    assert( (i16)GET_BITS(result, 0, 16) == (a >> sa) );
    assert( (i16)GET_BITS(result, 16, 16) == (b >> sa) );
}

__CX__ void pack_and_assert_add8(i8 a, i8 b, i8 c, i8 d, i8 e, i8 f, i8 g, i8 h) {
    u32 packed_i8_0 = pack_32_i8(a, b, c, d);
    u32 packed_i8_1 = pack_32_i8(e, f, g, h);
    u32 result = add8(packed_i8_0, packed_i8_1);
    assert( (i8)GET_BITS(result, 0, 8) == (a + e) );
    assert( (i8)GET_BITS(result, 8, 8) == (b + f) );
    assert( (i8)GET_BITS(result, 16, 8) == (c + g) );
    assert( (i8)GET_BITS(result, 24, 8) == (d + h) );
}

__CX__ void pack_and_assert_smul8(i8 a, i8 b, i8 c, i8 d, i8 e, i8 f, i8 g, i8 h) {
    u32 packed_i8_0 = pack_32_i8(a, b, c, d);
    u32 packed_i8_1 = pack_32_i8(e, f, g, h);
    u32 result = smul8(packed_i8_0, packed_i8_1);
    assert( (i8)GET_BITS(result, 0, 8) == (a * e) );
    assert( (i8)GET_BITS(result, 8, 8) == (b * f) );
    assert( (i8)GET_BITS(result, 16, 8) == (c * g) );
    assert( (i8)GET_BITS(result, 24, 8) == (d * h) );
}

__CX__ void pack_and_assert_sra8(i8 a, i8 b, i8 c, i8 d, i8 sa) {
    u32 packed_i8_0 = pack_32_i8(a, b, c, d);
    u32 result = sra8(packed_i8_0, sa);
    assert( (i8)GET_BITS(result, 0, 8) == (a >> sa) );
    assert( (i8)GET_BITS(result, 8, 8) == (b >> sa) );
    assert( (i8)GET_BITS(result, 16, 8) == (c >> sa) );
    assert( (i8)GET_BITS(result, 24, 8) == (d >> sa) );
}

void pext_test() {
    i16 a = 3, b = 5, c = 7, d = 9;
    u32 packed_i16_0, packed_i16_1;
    u32 result;

    cx_sel_t cx_sel_0 = cx_open( CX_GUID_PEXT, 0, -1 );
    assert(cx_sel_0 > 0);

    cx_sel( cx_sel_0 );

    /* =========================================
    *                16 Bit tests              
    *  =========================================
    */ 


    // ============== add16 ============== 

    pack_and_assert_add16(a, b, c, d);
    pack_and_assert_add16(-a, b, c, -d);
    pack_and_assert_add16(c, -d, -a, b);
    pack_and_assert_add16(-a, -b, -c, -d);


    // ============== smul16 ============== 

    pack_and_assert_smul16(a, b, c, d);
    pack_and_assert_smul16(-a, b, c, -d);
    pack_and_assert_smul16(c, -d, -a, b);
    pack_and_assert_smul16(-a, -b, -c, -d);

    // ============== sra16 ============== 

    pack_and_assert_sra16(a, b, c);
    pack_and_assert_sra16(-a, b, c);
    pack_and_assert_sra16(c, d, a);
    pack_and_assert_sra16(c, -d, a);

    
    /* =========================================
    *                8 Bit tests              
    *  =========================================
    */ 

    
    // ============== add8 ============== 

    i8 e = 1, f = 3, g = 5, h = 7, i = 9, j = 11, k = 13, l = 15;

    pack_and_assert_add8(e, f, g, h, i, j, k, l);
    pack_and_assert_add8(-e, -f, -g, -h, i, j, k, l);
    pack_and_assert_add8(e, f, g, h, -i, -j, -k, -l);
    pack_and_assert_add8(e, -f, g, -h, -i, j, -k, l);

    // ============== smul8 ============== 

    pack_and_assert_smul8(e, f, g, h, i, j, k, l);
    pack_and_assert_smul8(-e, -f, -g, -h, i, j, k, l);
    pack_and_assert_smul8(e, f, g, h, -i, -j, -k, -l);
    pack_and_assert_smul8(-e, f, -g, h, i, -j, k, -l);
    pack_and_assert_smul8(-e, -f, -g, -h, -i, -j, -k, -l);

    // ============== sra8 =============== 

    pack_and_assert_sra8(i, j, k, l, 1);
    pack_and_assert_sra8(i, j, k, l, 2);
    pack_and_assert_sra8(i, j, k, l, 3);
    pack_and_assert_sra8(i, j, k, l, 4);

    pack_and_assert_sra8(i, j, k, l, 1);
    pack_and_assert_sra8(-i, j, -k, l, 1);
    pack_and_assert_sra8(-i, -j, -k, -l, 2);
    pack_and_assert_sra8(i, j, k, l, 4);

    cx_close( cx_sel_0 );
    cx_sel( CX_LEGACY );
}

int main() {
    cx_sel( CX_LEGACY );
    pext_test();
    printf("p-ext test passed\n");
    return 0;
}