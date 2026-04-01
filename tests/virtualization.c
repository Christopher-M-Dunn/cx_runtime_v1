#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

void virtualization_test() {
  int a = 3;
  int b = 5;
  int c = 2;
  int result;

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
  int cx_sel_c1 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);

  assert(cx_sel_c1 > 0);

  cx_error_clear();
  cx_sel(cx_sel_c1);

  // Status is set to dirty after initializing
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );
  result = mac(b, b);
  assert( result == 25 );
  cx_error = cx_error_read();
//   assert( cx_error == 0 );
//   cx_status = CX_READ_STATUS();
//   assert( cx_status == expected_stctxs.idx );

  int cx_sel_c2 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
  assert(cx_sel_c2 > 0);

  cx_sel(cx_sel_c2);
  result = mac(a, a);
  assert( result == 9 );
  cx_error = cx_error_read();
//   assert( cx_error == 0 );
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  int cx_sel_c3 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
  assert(cx_sel_c3 > 0);

  cx_sel(cx_sel_c3);
  result = mac(c, c);
  assert( result == 4 );
  cx_error = cx_error_read();
//   assert( cx_error == 0 );
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  cx_sel(cx_sel_c2);
  result = mac(a, b);
  assert( result == 24 );
  cx_error = cx_error_read();
//   assert( cx_error == 0 );
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );

  cx_error_clear();
  cx_sel(cx_sel_c1);
  result = mac(a, c);
  assert( result == 31 );
  cx_error = cx_error_read();
//   assert( cx_error == 0 );
  cx_status = CX_READ_STATUS();
  assert( cx_status == expected_stctxs.idx );
  
  cx_sel_t cx_sel_c4 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
  assert( cx_sel_c4 > 0 );

  // Should still be using the same state as before
  result = mac(a, c);
  assert( result == 37 );

  cx_sel_t cx_sel_c5 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
  // Shouldn't be able to open another exclusive cx
  assert( cx_sel_c5 == -1 );

  cx_sel_t cx_sel_c6 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
  assert( cx_sel_c6 > 0 );


  cx_sel(cx_sel_c4);
  assert(CX_READ_STATE(0) == 0);
  result = mac(a, a);
  assert(result == 9);

  cx_close(cx_sel_c1);
  assert(cx_sel_c4 == cx_csr_read(CX_INDEX));
  result = mac(a, b);
  assert(result == 24);

  cx_close(cx_sel_c2);
  assert(cx_sel_c4 == cx_csr_read(CX_INDEX));
  result = mac(a, b);
  assert(result == 39);

  cx_close(cx_sel_c3);
  assert(cx_sel_c4 == cx_csr_read(CX_INDEX));
  result = mac(a, b);
  assert(result == 54);
  
  cx_close( cx_sel_c6 );

  // Making sure that the final close of the shared selector 
  // frees up the state.
  cx_sel_t cx_sel_c7 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
  assert( cx_sel_c7 > 0 );

  cx_close( cx_sel_c7 );
  cx_close( cx_sel_c5 );
  cx_close( cx_sel_c4 );

  cx_sel(CX_LEGACY);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    cx_sel(CX_LEGACY);
    virtualization_test();
    printf("Virtualization test complete\n");

    return 0;
}
