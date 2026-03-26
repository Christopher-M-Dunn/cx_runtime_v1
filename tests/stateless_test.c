#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/addsub/addsub.h"

void stateless_test() {
    int a = 3;
    int b = 5;
    int result;

    cx_share_t share_A = 0, share_C = 0;
    cx_error_t cx_error;

    int cx_sel_A0 = cx_open(CX_GUID_ADDSUB, share_A, -1);

    /* Index 0 should be reserved */
    assert( cx_sel_A0 > 0 );

    cx_error_clear();
    cx_sel(cx_sel_A0);

    result = add(a, b);
    assert( result == 8 );
    cx_error = cx_error_read();

    cx_close(cx_sel_A0);

    cx_sel_t cx_sel_A1 = cx_open(CX_GUID_ADDSUB, share_A, -1);
    cx_sel_t cx_sel_A2 = cx_open(CX_GUID_ADDSUB, share_A, -1);

    // The same index should be returned for stateless cxus
    assert( cx_sel_A1 > 0 );
    assert( cx_sel_A1 == cx_sel_A2 );
    cx_close( cx_sel_A1 );
    cx_close( cx_sel_A2 );
    cx_sel( CX_LEGACY );
}

int main() {

    // cx_sel( CX_LEGACY );
    stateless_test();
    printf("stateless test passed\n");

    return 0;
}
