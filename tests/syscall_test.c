/*
 * syscall_test.c — exercise every cx syscall added to the project
 *
 * Dumps MCX_SELECTOR, CX_INDEX, CX_STATUS, MCX_TABLE before starting,
 * between each syscall, and at the end.
 *
 * Syscalls under test (from unistd.h / syscall.tbl):
 *   471  cx_open(guid, share, share_sel)
 *   472  cx_close(sel)
 *   473  context_save()
 *   474  context_restore()
 *   475  do_nothing()
 *   476  test_syscall_brandon()
 */

#include <stdio.h>
#include <stdint.h>
#include "../include/ci.h"
#include "../include/utils.h"
#include "../zoo/mulacc/mulacc_common.h"

/* ------------------------------------------------------------------ */
/* Raw syscall wrappers for syscalls not in ci.c                       */
/* ------------------------------------------------------------------ */

static inline long sys_context_save(void) {
    register long ret  asm("a0");
    register long a7   asm("a7") = 473;
    asm volatile ("ecall" : "=r"(ret) : "r"(a7) : "memory");
    return ret;
}

static inline long sys_context_restore(void) {
    register long ret  asm("a0");
    register long a7   asm("a7") = 474;
    asm volatile ("ecall" : "=r"(ret) : "r"(a7) : "memory");
    return ret;
}

static inline long sys_do_nothing(void) {
    register long ret  asm("a0");
    register long a7   asm("a7") = 475;
    asm volatile ("ecall" : "=r"(ret) : "r"(a7) : "memory");
    return ret;
}

static inline long sys_test_syscall_brandon(void) {
    register long ret  asm("a0");
    register long a7   asm("a7") = 476;
    asm volatile ("ecall" : "=r"(ret) : "r"(a7) : "memory");
    return ret;
}

/* ------------------------------------------------------------------ */
/* CSR dump                                                            */
/* ------------------------------------------------------------------ */

static void dump_csrs(const char *label) {
    // uint mcx_sel  = cx_csr_read(MCX_SELECTOR); // privileged (S-mode only)
    uint cx_idx   = cx_csr_read(CX_INDEX);
    uint cx_stat  = cx_csr_read(CX_STATUS);
    // uint mcx_tbl  = cx_csr_read(MCX_TABLE);    // privileged (S-mode only)
    printf("[CSR dump: %s]\n", label);
    // printf("  MCX_SELECTOR = 0x%08x\n", mcx_sel);
    printf("  CX_INDEX  = 0x%08x  (%u)\n", cx_idx, cx_idx);
    printf("  CX_STATUS = 0x%08x\n", cx_stat);
    // printf("  MCX_TABLE    = 0x%08x\n", mcx_tbl);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    printf("=== syscall_test start ===\n\n");

    dump_csrs("initial");

    /* 471: cx_open — open a mulacc context (stateful, CX_NO_VIRT) */
    printf("\n--- cx_open (471) ---\n");
    cx_sel_t sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    printf("  cx_open returned: %d\n", sel);
    dump_csrs("after cx_open");

    /* 475: do_nothing — first-use trap handler */
    printf("\n--- do_nothing (475) ---\n");
    long dn_ret = sys_do_nothing();
    printf("  do_nothing returned: %ld\n", dn_ret);
    dump_csrs("after do_nothing");

    /* 476: test_syscall_brandon */
    printf("\n--- test_syscall_brandon (476) ---\n");
    long tsb_ret = sys_test_syscall_brandon();
    printf("  test_syscall_brandon returned: %ld\n", tsb_ret);
    dump_csrs("after test_syscall_brandon");

    /* 473: context_save */
    printf("\n--- context_save (473) ---\n");
    long cs_ret = sys_context_save();
    printf("  context_save returned: %ld\n", cs_ret);
    dump_csrs("after context_save");

    /* 474: context_restore */
    printf("\n--- context_restore (474) ---\n");
    long cr_ret = sys_context_restore();
    printf("  context_restore returned: %ld\n", cr_ret);
    dump_csrs("after context_restore");

    /* 472: cx_close */
    printf("\n--- cx_close (472) ---\n");
    if (sel > 0) {
        cx_close(sel);
        printf("  cx_close(%d) called\n", sel);
    } else {
        printf("  skipped (sel=%d invalid)\n", sel);
    }
    dump_csrs("after cx_close");

    printf("\n=== syscall_test end ===\n");
    return 0;
}