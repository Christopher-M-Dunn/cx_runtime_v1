#include <stdint.h>
#include <stdlib.h>

#include "../include/ci.h"

int32_t cx_open(cx_guid_t cx_guid, cx_share_t cx_share, cx_sel_t cx_sel) {

  register long cx_index asm("a0");
  register long a0 asm("a0") = cx_guid;
  register long a1 asm("a1") = cx_share;
  register long a2 asm("a2") = cx_sel;
  register long syscall_id asm("a7") = 471; // cx_open
  asm volatile ("ecall  # 0=%0   1=%1  2=%2  3=%3 4=%4"
    : "=r"(cx_index)
    : "r"(a0), "r"(a1), "r"(a2), "r"(syscall_id)
    :
  );
  return cx_index;
}

void cx_close(cx_sel_t cx_sel)
{
  int cx_close_error = -1;
  asm volatile (
    "li a7, 472;        \n\t"  // cx_close
    "mv a0, %0;         \n\t"  // a0-a5 are ecall args 
    "ecall;             \n\t"
    "mv %1, a0;         \n\t"
    :  "=r" (cx_close_error)
    :  "r"  (cx_sel)  
    : 
  );
}

cx_error_t cx_error_read() {
  return cx_csr_read(CX_STATUS);
}

void cx_error_clear() {
  cx_csr_write(CX_STATUS, 0);
}

void cx_deselect_and_close(cx_sel_t cx_sel)
{
    return;
}
