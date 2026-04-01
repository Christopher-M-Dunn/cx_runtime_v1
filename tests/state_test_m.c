#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"
#include "../zoo/addsub/addsub.h"
#include "../zoo/p-ext/p-ext.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

static cx_stctxs_t expected_stctxs = {.sel = {
                                .dc = CX_DIRTY,
                                .R = 0,
                                .state_size = 1,
                                .version = 1
                              }};

void state_test() {
    int a = 3;
    int b = 5;
    int result;

    cx_sel_t mcx_selector;

    /* cx_error should be set to 0 initially */
    uint cx_error = cx_error_read();
    assert ( cx_error == 0 );

    int cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert ( cx_sel_A0 == 0x20000002 );

    cx_error_clear();
    cx_sel(cx_sel_A0);

    uint status = CX_READ_STATUS();
    
    assert (status == expected_stctxs.idx);

    result = mac(a, b);
    assert( result == 15 );

    status = CX_READ_STATUS();

    assert (status == expected_stctxs.idx);

    cx_error = cx_error_read();
    assert ( cx_error == 0 );

    cx_close(cx_sel_A0);

    /* Testing multiple states */
    int cx_sel_A1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    int cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_A1 == 0x20000002 );
    assert( cx_sel_A2 == 0x20010002 );
    int cx_sel_A3 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_A3 == -1 );

    cx_error_clear();
    cx_sel(cx_sel_A1);

    result = mac(a, a);
    assert( result == 9 );
    cx_error = cx_error_read();
    assert ( cx_error == 0 );

    cx_error_clear();
    cx_sel(cx_sel_A2);
    result = mac(b, b);
    assert( result == 25 );
    cx_error = cx_error_read();
    assert ( cx_error == 0 );

    cx_close(cx_sel_A1);

    uint cx_sel_test = -1;

    const int INVALID_CX_GUID = 0;
    int cx_sel_invalid = cx_open(INVALID_CX_GUID, CX_NO_VIRT, -1);
    
    assert( cx_sel_invalid == -1 );
    cx_sel( CX_LEGACY );
}

uint32_t pack_32_i16(int16_t a, int16_t b) {
    int32_t res = (a & 0xFFFF);
    res |= (b << 16) & 0xFFFF0000;
    return res;
}

void system_test() {
    const int a = 3, b = 5;
    cx_sel_t addsub_sel_0 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( addsub_sel_0 == 0x20000000 );
    cx_sel_t addsub_sel_1 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( addsub_sel_0 == 0x20000000 );
    cx_sel_t addsub_sel_2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( addsub_sel_0 == 0x20000000 );
    cx_sel_t addsub_sel_3 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert( addsub_sel_0 == 0x20000000 );

    cx_sel_t p_sel_0 = cx_open(CX_GUID_PEXT, CX_NO_VIRT, -1);
    assert( p_sel_0 == 0x20000003 );

    assert(add(a, b) == 8);
    assert(sub(a, b) == -2);

    cx_sel(addsub_sel_0);

    uint32_t packed_a = pack_32_i16(a, b);
    uint32_t packed_b = pack_32_i16(a, a);

    cx_sel(p_sel_0);
    uint result = add16(packed_a, packed_b);
    assert((int16_t)GET_BITS(result, 0, 16) == (a + a));
    assert((int16_t)GET_BITS(result, 16, 16) == (a + b));
    
    cx_close(addsub_sel_0);
    cx_close(addsub_sel_1);
    cx_close(addsub_sel_2);
    cx_close(addsub_sel_3);
    cx_close(p_sel_0);
    cx_sel( CX_LEGACY );

}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    cx_init();
    cx_sel( CX_LEGACY );
    state_test();
    printf("state test passed (m mode)\n");
    return 0;
}
