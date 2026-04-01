/*
 * if_flag_test.c — exhaustive IC/IF/IS flag test
 *
 * Flags (cx_status_t, CX_STATUS CSR 0x801):
 *   IV = bit 0  invalid version
 *   IC = bit 1  invalid selector
 *   IS = bit 2  invalid state
 *   IF = bit 4  invalid function (CF ID)
 *
 * Key behaviours confirmed from source:
 *   - Flags are cumulative: each check ORs into existing CX_STATUS
 *   - CX_INVALID_SELECTOR → IV+IC+IS, EARLY RETURN (no further checks)
 *   - All other error paths fall through to cx_funcs call
 *   - cx_error_read() does NOT clear flags; must call cx_error_clear()
 *   - CX_READ_STATUS (CF 1023) and CX_STATUS (0x801) are separate
 *   - CF ID is a 10-bit immediate (0..1023): the assembler enforces this,
 *     so OPCODE_ID >= MAX_CF_IDS (1024) is unreachable from a normal binary.
 *     The >= MAX_CF_IDS branch in cx_helper.c can only be triggered by a
 *     malformed/injected instruction word.
 *
 * Test groups:
 *   A. Stateful CXU (mulacc): valid CFs, out-of-range, system CFs
 *   B. Stateless CXU (addsub): valid CFs, out-of-range, system CFs
 *   C. Cumulative flag accumulation
 *   D. Invalid selector (IV+IC+IS, no IF)
 *   E. Stateless + Stateful muldiv cross-check
 */

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "../include/ci.h"
#include "../include/utils.h"
#include "../zoo/mulacc/mulacc_common.h"
#include "../zoo/addsub/addsub_common.h"
#include "../zoo/muldiv/muldiv_common.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

static int pass_count = 0;
static int fail_count = 0;

#define CHECK(label, expr) do { \
    int _v = (expr); \
    if (_v) { printf("  [PASS] %s\n", label); pass_count++; } \
    else    { printf("  [FAIL] %s\n", label); fail_count++; } \
} while(0)

/* Read full CX_STATUS and decode all flags */
static cx_status_t read_status(void) {
    cx_status_t s = {.idx = cx_csr_read(CX_STATUS)};
    return s;
}

static void print_status(const char *label) {
    cx_status_t s = read_status();
    VLOG("  [%s] CX_STATUS=0x%08x  IV=%u IC=%u IS=%u OF=%u IF=%u OP=%u CU=%u\n",
         label, s.idx, s.sel.IV, s.sel.IC, s.sel.IS,
         s.sel.OF, s.sel.IF, s.sel.OP, s.sel.CU);
}

/* cx_reg requires an immediate CF ID — one helper per value under test */
static inline int32_t cf_mac(void)       { return CX_REG_HELPER(0,    1, 1); } /* mulacc CF 0 */
static inline int32_t cf_mac_last(void)  { return CX_REG_HELPER(4,    0, 0); } /* mulacc CF 4 (last valid) */
static inline int32_t cf_mac_oob(void)   { return CX_REG_HELPER(5,    0, 0); } /* mulacc CF 5 (one past last) */
static inline int32_t cf_add(void)       { return CX_REG_HELPER(0,    2, 3); } /* addsub CF 0 (add, valid) */
static inline int32_t cf_add_null(void)  { return CX_REG_HELPER(1,    0, 0); } /* addsub CF 1 (NULL slot) */
static inline int32_t cf_add_sub(void)   { return CX_REG_HELPER(2,    5, 3); } /* addsub CF 2 (sub, valid) */
static inline int32_t cf_add_null2(void) { return CX_REG_HELPER(3,    0, 0); } /* addsub CF 3 (NULL slot) */
static inline int32_t cf_add_last(void)  { return CX_REG_HELPER(4,    0, 0); } /* addsub CF 4 (add_1000, last valid) */
static inline int32_t cf_add_oob(void)   { return CX_REG_HELPER(5,    0, 0); } /* addsub CF 5 (one past last) */
static inline int32_t cf_mid_oob(void)   { return CX_REG_HELPER(500,  0, 0); } /* mid-range, unregistered */
static inline int32_t cf_sys_1020(void)  { return CX_REG_HELPER(1020, 0, 0); } /* write_state */
static inline int32_t cf_sys_1021(void)  { return CX_REG_HELPER(1021, 0, 0); } /* read_state */
static inline int32_t cf_sys_1022(void)  { return CX_REG_HELPER(1022, 0, 0); } /* write_status */
static inline int32_t cf_sys_1023(void)  { return CX_REG_HELPER(1023, 0, 0); } /* read_status */
static inline int32_t cf_sys_1019(void)  { return CX_REG_HELPER(1019, 0, 0); } /* just below system range */

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v') verbose = 1;
    }

    printf("=== if_flag_test ===\n");
    printf("    MAX_CF_IDS=%d  mulacc_funcs=%d  addsub_funcs=%d\n\n",
           MAX_CF_IDS, CX_MULACC_NUM_FUNCS, CX_ADDSUB_NUM_FUNCS);

    cx_sel_t sel_mac, sel_as, sel_md;

    /* ------------------------------------------------------------------ */
    printf("--- A. Stateful CXU (mulacc, CFs 0-%d, system 1020-1023) ---\n",
           CX_MULACC_NUM_FUNCS - 1);

    sel_mac = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(sel_mac > 0);
    cx_sel(sel_mac);

    cx_error_clear();
    cf_mac();
    print_status("after CF 0 (valid)");
    CHECK("A1: stateful valid CF 0 -- no flags", read_status().idx == 0);

    cx_error_clear();
    cf_mac_last();
    print_status("after CF 4 (last valid)");
    CHECK("A2: stateful last valid CF 4 -- no flags", read_status().idx == 0);

    cx_error_clear();
    cf_mac_oob();
    print_status("after CF 5 (one past last)");
    CHECK("A3: stateful CF 5 (oob) -- IF set",   read_status().sel.IF == 1);
    CHECK("A3: stateful CF 5 (oob) -- IC clear", read_status().sel.IC == 0);
    CHECK("A3: stateful CF 5 (oob) -- IS clear", read_status().sel.IS == 0);

    cx_error_clear();
    cf_mid_oob();
    print_status("after CF 500 (mid, unregistered)");
    CHECK("A4: stateful CF 500 (mid oob) -- IF set", read_status().sel.IF == 1);

    cx_error_clear();
    cf_sys_1019();
    print_status("after CF 1019 (just below system)");
    CHECK("A5: stateful CF 1019 (unregistered) -- IF set", read_status().sel.IF == 1);

    cx_error_clear();
    cf_sys_1020();
    print_status("after CF 1020 (write_state, registered)");
    CHECK("A6: stateful CF 1020 (write_state) -- no flags", read_status().idx == 0);

    cx_error_clear();
    cf_sys_1021();
    print_status("after CF 1021 (read_state, registered)");
    CHECK("A7: stateful CF 1021 (read_state) -- no flags", read_status().idx == 0);

    cx_error_clear();
    cf_sys_1022();
    print_status("after CF 1022 (write_status, registered)");
    CHECK("A8: stateful CF 1022 (write_status) -- no flags", read_status().idx == 0);

    cx_error_clear();
    cf_sys_1023();
    print_status("after CF 1023 (read_status, registered)");
    CHECK("A9: stateful CF 1023 (read_status) -- no flags", read_status().idx == 0);

    cx_close(sel_mac);
    cx_sel(CX_LEGACY);

    /* ------------------------------------------------------------------ */
    printf("\n--- B. Stateless CXU (addsub, CFs 0-%d, holes at 1 and 3, no system CFs) ---\n",
           CX_ADDSUB_NUM_FUNCS - 1);

    sel_as = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert(sel_as > 0);
    cx_sel(sel_as);

    cx_error_clear();
    cf_add();
    print_status("after CF 0 (add, valid)");
    CHECK("B1: stateless valid CF 0 -- no flags", read_status().idx == 0);

    cx_error_clear();
    cf_add_null();
    print_status("after CF 1 (NULL slot)");
    CHECK("B2: stateless CF 1 (NULL) -- IF set",   read_status().sel.IF == 1);
    CHECK("B2: stateless CF 1 (NULL) -- IC clear", read_status().sel.IC == 0);
    CHECK("B2: stateless CF 1 (NULL) -- IS clear", read_status().sel.IS == 0);

    cx_error_clear();
    cf_add_sub();
    print_status("after CF 2 (sub, valid)");
    CHECK("B3: stateless valid CF 2 -- no flags", read_status().idx == 0);

    cx_error_clear();
    cf_add_null2();
    print_status("after CF 3 (NULL slot)");
    CHECK("B4: stateless CF 3 (NULL) -- IF set",   read_status().sel.IF == 1);
    CHECK("B4: stateless CF 3 (NULL) -- IC clear", read_status().sel.IC == 0);
    CHECK("B4: stateless CF 3 (NULL) -- IS clear", read_status().sel.IS == 0);

    cx_error_clear();
    cf_add_last();
    print_status("after CF 4 (add_1000, last valid)");
    CHECK("B5: stateless last valid CF 4 -- no flags", read_status().idx == 0);

    cx_error_clear();
    cf_add_oob();
    print_status("after CF 5 (one past last)");
    CHECK("B6: stateless CF 5 (oob) -- IF set",   read_status().sel.IF == 1);
    CHECK("B6: stateless CF 5 (oob) -- IC clear", read_status().sel.IC == 0);
    CHECK("B6: stateless CF 5 (oob) -- IS clear", read_status().sel.IS == 0);

    cx_error_clear();
    cf_mid_oob();
    print_status("after CF 500 (mid, unregistered)");
    CHECK("B7: stateless CF 500 (mid oob) -- IF set", read_status().sel.IF == 1);

    cx_error_clear();
    cf_sys_1019();
    print_status("after CF 1019 (just below system, unregistered on stateless)");
    CHECK("B8: stateless CF 1019 -- IF set", read_status().sel.IF == 1);

    cx_error_clear();
    cf_sys_1023();
    print_status("after CF 1023 (not registered on stateless)");
    CHECK("B9: stateless CF 1023 (not registered) -- IF set", read_status().sel.IF == 1);

    cx_close(sel_as);
    cx_sel(CX_LEGACY);

    /* ------------------------------------------------------------------ */
    printf("\n--- C. Flag accumulation ---\n");

    sel_mac = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(sel_mac > 0);
    cx_sel(sel_mac);

    /* Fire IF twice without clearing — should remain set */
    cx_error_clear();
    cf_mac_oob();
    VLOG("  after first oob: IF=%u\n", read_status().sel.IF);
    cf_mac_oob();
    print_status("after second oob (no clear in between)");
    CHECK("C1: IF stays set across two oob calls without clear", read_status().sel.IF == 1);

    /* Valid call after IF set — IF should still be set (not cleared by cx_funcs) */
    cf_mac();
    print_status("after valid CF 0 following IF");
    CHECK("C2: IF persists after valid call (not auto-cleared)", read_status().sel.IF == 1);

    /* cx_error_clear() clears all */
    cx_error_clear();
    print_status("after cx_error_clear");
    CHECK("C3: cx_error_clear clears IF", read_status().sel.IF == 0);
    CHECK("C3: cx_error_clear clears all flags", read_status().idx == 0);

    cx_close(sel_mac);
    cx_sel(CX_LEGACY);

    /* ------------------------------------------------------------------ */
    printf("\n--- D. Invalid selector (CX_INVALID_SELECTOR) ---\n");

    /* Force mcx_selector to CX_INVALID_SELECTOR by writing a closed sel index.
     * After cx_close the mcx_table entry is CX_INVALID_SELECTOR; cx_sel(idx)
     * writes that to CX_INDEX/MCX_SELECTOR. */
    sel_mac = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(sel_mac > 0);
    cx_close(sel_mac);
    cx_error_clear();
    cx_sel(sel_mac);  /* writes CX_INDEX to the now-invalid slot */
    cf_mac();
    cx_status_t sd = read_status();
    print_status("after CF 0 with invalid selector");
    CHECK("D1: invalid selector -- IC set",       sd.sel.IC == 1);
    CHECK("D1: invalid selector -- IV set",       sd.sel.IV == 1);
    CHECK("D1: invalid selector -- IS set",       sd.sel.IS == 1);
    CHECK("D1: invalid selector -- IF NOT set (early return)", sd.sel.IF == 0);

    cx_sel(CX_LEGACY);
    cx_error_clear();

    /* ------------------------------------------------------------------ */
    printf("\n--- E. Stateless + Stateful muldiv cross-check ---\n");

    sel_md = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    assert(sel_md > 0);
    cx_sel(sel_md);

    cx_error_clear();
    CX_REG_HELPER(0, 4, 5);  /* mul: CF 0 */
    print_status("after muldiv CF 0 (mul, valid)");
    CHECK("E1: muldiv valid CF 0 -- no flags", read_status().idx == 0);

    cx_error_clear();
    CX_REG_HELPER(2, 0, 0);  /* CF 2: one past last (muldiv has 2 funcs) */
    print_status("after muldiv CF 2 (oob)");
    CHECK("E2: muldiv CF 2 (oob) -- IF set", read_status().sel.IF == 1);

    cx_error_clear();
    cf_sys_1023();  /* not registered on muldiv */
    print_status("after muldiv CF 1023 (not registered)");
    CHECK("E3: muldiv CF 1023 (not registered) -- IF set", read_status().sel.IF == 1);

    cx_close(sel_md);
    cx_sel(CX_LEGACY);

    /* ------------------------------------------------------------------ */
    printf("\n========================================\n");
    printf("  RESULTS: %d passed, %d failed\n", pass_count, fail_count);
    printf("========================================\n");
    return fail_count ? 1 : 0;
}