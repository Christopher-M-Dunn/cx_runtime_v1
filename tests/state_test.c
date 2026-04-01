#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

#define CX_SEL_TABLE_NUM_ENTRIES 1024

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

    cx_share_t share_A = 0, share_C = 0;

    /* cx_index should be set to 0 initially */
    VLOG("reading CX_INDEX (expect 0)...\n");
    uint cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert ( cx_index == 0 );

    /* cx_error should be set to 0 initially */
    VLOG("reading cx_error (expect 0)...\n");
    uint cx_error = cx_error_read();
    VLOG("  cx_error = 0x%08x\n", cx_error);
    assert ( cx_error == 0 );

    VLOG("cx_open(MULACC, NO_VIRT, -1)...\n");
    int cx_sel_A0 = cx_open(CX_GUID_MULACC, share_A, -1);
    VLOG("  cx_sel_A0 = %d\n", cx_sel_A0);

    /* cx_open should not modify the selected cxu */
    VLOG("reading CX_INDEX after cx_open (expect 0)...\n");
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert ( cx_index == 0 );

    /* Index 0 should be reserved */
    assert( cx_sel_A0 > 0 );

    cx_error_clear();
    VLOG("cx_sel(%d)...\n", cx_sel_A0);
    cx_sel(cx_sel_A0);
    VLOG("reading CX_INDEX (expect %d)...\n", cx_sel_A0);
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert ( cx_index == cx_sel_A0 );

    VLOG("CX_READ_STATUS()...\n");
    uint status = CX_READ_STATUS();
    uint dc = GET_CX_DATA_CLEAN(status);
    uint state_size = GET_CX_STATE_SIZE(status);
    uint reset = GET_CX_RESET(status);
    VLOG("  status=0x%08x  dc=%u  state_size=%u  reset=%u\n", status, dc, state_size, reset);

    /* Status should always be dirty after either a hw or sw initialization */
    assert( dc == CX_DIRTY );
    assert( state_size == 1 );  // state size unchanged
    assert( reset == 0 ); // initializer unchanged

    VLOG("mac(%d, %d)...\n", a, b);
    result = mac(a, b);
    VLOG("  result = %d (expect 15)\n", result);
    assert( result == 15 );

    VLOG("CX_READ_STATUS() after mac...\n");
    status = CX_READ_STATUS();
    VLOG("  status=0x%08x  expected=0x%08x\n", status, expected_stctxs.idx);
    assert (status == expected_stctxs.idx);

    VLOG("cx_error_read()...\n");
    cx_error = cx_error_read();
    VLOG("  cx_error = 0x%08x (expect 0)\n", cx_error);
    assert ( cx_error == 0 );

    VLOG("cx_close(%d)...\n", cx_sel_A0);
    cx_close(cx_sel_A0);
    VLOG("  closed\n");

    /* Testing multiple states */
    VLOG("cx_open A1 (MULACC, NO_VIRT, -1)...\n");
    int cx_sel_A1 = cx_open(CX_GUID_MULACC, share_A, -1);
    VLOG("  cx_sel_A1 = %d\n", cx_sel_A1);
    VLOG("reading CX_INDEX after A1 open (expect %d)...\n", cx_sel_A0);
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert( cx_index == cx_sel_A0 );

    VLOG("cx_open A2 (MULACC, NO_VIRT, -1)...\n");
    int cx_sel_A2 = cx_open(CX_GUID_MULACC, share_A, -1);
    VLOG("  cx_sel_A2 = %d\n", cx_sel_A2);
    VLOG("reading CX_INDEX after A2 open (expect %d)...\n", cx_sel_A0);
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert( cx_index == cx_sel_A0 );

    assert( cx_sel_A1 > 0 );
    assert( cx_sel_A2 > 0 );

    cx_error_clear();
    VLOG("cx_sel(%d)...\n", cx_sel_A1);
    cx_sel(cx_sel_A1);
    VLOG("reading CX_INDEX (expect %d)...\n", cx_sel_A1);
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert ( cx_index == cx_sel_A1 );

    VLOG("mac(%d, %d) on A1...\n", a, a);
    result = mac(a, a);
    VLOG("  result = %d (expect 9)\n", result);
    assert( result == 9 );

    VLOG("cx_error_read()...\n");
    cx_error = cx_error_read();
    VLOG("  cx_error = 0x%08x (expect 0)\n", cx_error);
    assert ( cx_error == 0 );

    cx_error_clear();
    VLOG("cx_sel(%d)...\n", cx_sel_A2);
    cx_sel(cx_sel_A2);
    VLOG("reading CX_INDEX (expect %d)...\n", cx_sel_A2);
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert ( cx_index == cx_sel_A2 );

    VLOG("mac(%d, %d) on A2...\n", b, b);
    result = mac(b, b);
    VLOG("  result = %d (expect 25)\n", result);
    assert( result == 25 );

    VLOG("cx_error_read()...\n");
    cx_error = cx_error_read();
    VLOG("  cx_error = 0x%08x (expect 0)\n", cx_error);
    assert ( cx_error == 0 );

    VLOG("cx_close(%d) (A1)...\n", cx_sel_A1);
    cx_close(cx_sel_A1);
    VLOG("reading CX_INDEX after A1 close (expect %d)...\n", cx_sel_A2);
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert ( cx_index == cx_sel_A2 );

    uint cx_sel_test = -1;
    // Making sure free states are able to be used again
    VLOG("cycling %d open/close pairs...\n", CX_SEL_TABLE_NUM_ENTRIES - 4);
    for (int i = 0; i < CX_SEL_TABLE_NUM_ENTRIES - 4; i++) {
        cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
        cx_close(cx_sel_test);
    }
    VLOG("  done cycling\n");

    VLOG("cx_open (MULACC)...\n");
    cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
    VLOG("  cx_sel_test = %u (expect > 0)\n", cx_sel_test);
    assert( cx_sel_test > 0 );
    VLOG("reading CX_INDEX (expect %d)...\n", cx_sel_A2);
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert ( cx_index == cx_sel_A2 );

    VLOG("cx_close(%u)...\n", cx_sel_test);
    cx_close(cx_sel_test);
    VLOG("reading CX_INDEX (expect %d)...\n", cx_sel_A2);
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert ( cx_index == cx_sel_A2 );

    VLOG("cx_open (MULACC)...\n");
    cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
    VLOG("  cx_sel_test = %u (expect > 0)\n", cx_sel_test);
    assert( cx_sel_test > 0 );
    VLOG("reading CX_INDEX (expect %d)...\n", cx_sel_A2);
    cx_index = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX = %u\n", cx_index);
    assert ( cx_index == cx_sel_A2 );

    VLOG("cx_close(%u)...\n", cx_sel_test);
    cx_close(cx_sel_test);

    VLOG("cx_open (MULACC) then immediately close...\n");
    cx_sel_test = cx_open(CX_GUID_MULACC, 0, -1);
    VLOG("  cx_sel_test = %u\n", cx_sel_test);
    cx_close(cx_sel_test);

    // cx_index 3 is still in use
    assert( cx_sel_test > 0 );

    VLOG("cx_close(%d) (A2)...\n", cx_sel_A2);
    cx_close(cx_sel_A2);

    const int INVALID_CX_GUID = 0;
    VLOG("cx_open(guid=0, invalid) (expect -1)...\n");
    int cx_sel_invalid = cx_open(INVALID_CX_GUID, share_A, -1);
    VLOG("  cx_sel_invalid = %d\n", cx_sel_invalid);
    assert( cx_sel_invalid == -1 );

    cx_sel( CX_LEGACY );
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    cx_sel( CX_LEGACY );
    state_test();
    printf("state test passed\n");
    return 0;
}