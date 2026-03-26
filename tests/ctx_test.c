#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

void context_save_restore_test() {
  int a = 3;
  int b = 5;
  int c = 2;
  int result;

  cx_share_t share_A = 0, share_C = 0;
  cx_error_t cx_error;
  uint cx_status;
  int32_t state_result;
  cx_sel_t cx_index;

  static cx_stctxs_t expected_stctxs = {.sel = {
                                .dc = CX_DIRTY,
                                .R = 0,
                                .state_size = 1,
                                .version = 1
                              }};

  int cx_sel_C0 = cx_open(CX_GUID_MULACC, share_C, -1);
  int cx_sel_C1 = cx_open(CX_GUID_MULACC, share_C, -1);

  assert(cx_sel_C0 > 0);
  assert(cx_sel_C1 == cx_sel_C0 + 1);

  cx_error_clear();
  cx_sel(cx_sel_C1);
  result = mac(b, b);
  assert( result == 25 );
  cx_error = cx_error_read();
  assert( cx_error == 0 );
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  cx_error_clear();
  cx_sel(cx_sel_C0);
  result = mac(a, a);
  assert(result == 9);
  cx_error = cx_error_read();
  assert( cx_error == 0 );
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  cx_context_save();

  // accumulators will have been reset
  // filling new state with info
  result = mac(a, b);
  assert( result == 15 );
  state_result = CX_READ_STATE(0);
  assert( result == state_result );
  cx_error = cx_error_read();
  assert( cx_error == 0 );
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  cx_error_clear();
  cx_sel(cx_sel_C1);
  result = mac(c, c);
  assert( result == 4 );
  cx_error = cx_error_read();
  assert( cx_error == 0 );
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  cx_context_restore();
  // checking cx_index was saved
  cx_index = cx_csr_read(CX_INDEX);
  assert( cx_index == cx_sel_C0 );

  // checking error was restored
  cx_error = cx_error_read();
  assert( cx_error == 0 ); 

  // checking state data was restored
  state_result = CX_READ_STATE(0);
  assert( state_result == 9 );

  // checking state status was restored, and set to clean
  cx_status = CX_READ_STATUS();
  expected_stctxs.sel.dc = CX_CLEAN;
  assert( cx_status == expected_stctxs.idx );

  cx_error_clear();
  // checking the other state
  cx_sel(cx_sel_C1);
  state_result = CX_READ_STATE(0);
  assert( state_result == 25 );
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  cx_close(cx_sel_C0);
  cx_close(cx_sel_C1);

  cx_sel(CX_LEGACY);
}

int main() {
    cx_sel(CX_LEGACY);
    context_save_restore_test();
    printf("Context save / restore test complete\n");

    return 0;
}
