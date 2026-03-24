#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

void state_test() {
    int a = 3;
    int b = 5;
    int result;

    /* cx_index should be set to 0 initially */
    uint cx_index = cx_csr_read(MCX_SELECTOR);
    assert ( cx_index == 0 );

    /* cx_error should be set to 0 initially */
    uint cx_error = cx_error_read();
    assert ( cx_error == 0 );

    int cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    /* cx_open should not modify the selected cxu */
    cx_index = cx_csr_read(MCX_SELECTOR);
    assert ( cx_index == 0 );
    /* Index 0 should be reserved */
    assert( cx_sel_A0 > 0 );

    cx_error_clear();
    cx_sel(cx_sel_A0);
    cx_index = cx_csr_read(MCX_SELECTOR);
    assert ( cx_index == cx_sel_A0 );
    uint status = CX_READ_STATUS();
    // uint cs_status = GET_CX_STATUS(status);
    uint state_size = GET_CX_STATE_SIZE(status);
    // uint error = GET_CX_ERROR(status);
    // uint initializer_cfg = GET_CX_INITIALIZER(status);

    /* Status should always be dirty after either a hw or sw initialization */
    // assert( cs_status == CX_DIRTY );
    // assert( state_size == 1 );  // state size unchanged
    // assert( error == 0 );       // No error
    // assert( initializer_cfg == 0 ); // initializer unchanged
    result = mac(a, b);
    assert( result == 15 );
    status = CX_READ_STATUS();

    // cs_status = GET_CX_STATUS(status);
    state_size = GET_CX_STATE_SIZE(status);
    // error = GET_CX_ERROR(status);
    // initializer_cfg = GET_CX_INITIALIZER(status);
    // assert( cs_status == CX_DIRTY );
    // assert( state_size == 1 );  // state size unchanged
    // assert( error == 0 );       // No error
    // assert( initializer_cfg == 0 ); // initializer unchanged

    cx_error = cx_error_read();
    assert ( cx_error == 0 );

    cx_close(cx_sel_A0);

    /* Testing multiple states */
    int cx_sel_A1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_index = cx_csr_read(MCX_SELECTOR);
    // assert( cx_index == CX_LEGACY );
    int cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_index = cx_csr_read(MCX_SELECTOR);
    // assert( cx_index == CX_LEGACY );

    assert( cx_sel_A1 > 0 );
    assert( cx_sel_A2 > 0 );

    cx_error_clear();
    cx_sel(cx_sel_A1);
    cx_index = cx_csr_read(MCX_SELECTOR);
    assert ( cx_index == cx_sel_A1 );
    result = mac(a, a);
    assert( result == 9 );
    cx_error = cx_error_read();
    assert ( cx_error == 0 );
    cx_error_clear();
    cx_sel(cx_sel_A2);
    cx_index = cx_csr_read(MCX_SELECTOR);
    assert ( cx_index == cx_sel_A2 );
    result = mac(b, b);
    assert( result == 25 );
    cx_error = cx_error_read();
    assert ( cx_error == 0 );

    cx_close(cx_sel_A1);
    cx_index = cx_csr_read(MCX_SELECTOR);
    // assert ( cx_index == CX_LEGACY );

    uint cx_sel_test = -1;

    cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
    assert( cx_sel_test > 0 );
    cx_index = cx_csr_read(MCX_SELECTOR);
    // assert ( cx_index == CX_LEGACY );

    cx_close(cx_sel_test);
    cx_index = cx_csr_read(MCX_SELECTOR);
    // assert ( cx_index == CX_LEGACY );

    cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
    assert( cx_sel_test > 0 );
    cx_index = cx_csr_read(MCX_SELECTOR);
    // assert ( cx_index == CX_LEGACY );

    cx_close(cx_sel_test);

    cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
    cx_close(cx_sel_test);

    // cx_index 3 is still in use
    assert( cx_sel_test > 0 );

    cx_close(cx_sel_A2);

    const int INVALID_CX_GUID = -1;
    cx_sel_test = cx_open(INVALID_CX_GUID, CX_NO_VIRT, -1);
    assert( cx_sel_test == -1 );
    cx_sel( CX_LEGACY );
}

int main() {
    cx_sel( CX_LEGACY );
    state_test();
    printf("state test passed\n");
    return 0;
}