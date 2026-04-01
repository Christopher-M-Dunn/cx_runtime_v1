/*
 * testbench.c — verbose CX diagnostic and mode test
 *
 * Author: Christopher M. Dunn
 * Date:   26 MAR 2026
 *
 * Notes: This is not an exhaustive test, and may have some logic errors
 *
 * Tests:
 *   1. Initial CSR state
 *   2. cx_open / cx_close for every CXU (including invalid GUID)
 *   3. Stateless CXUs (addsub, muldiv): operations + div/0 diagnostic + multi-open
 *   4. Stateful CXU (mulacc) CX_NO_VIRT: isolation, state exhaustion, INTRA_VIRT fallback
 *   5. CX_INTRA_VIRT: independent contexts (LRU spill) + explicit sharing via share_sel
 *   6. State reset on close + re-open; multi-open refcount
 *   7. CX_INDEX CSR tracks cx_sel() correctly
 *   8. MCX_SELECTOR vs CX_INDEX after cx_sel()
 *   9. cx_open does not disturb active CX_INDEX
 *  10. Error cases
 *
 * CX_CS dc field values (from CX_READ_STATUS / cx_stctxs_t.sel.dc):
 *   CX_OFF      (0): never initialized; using it sets IS error bit
 *   CX_PRECLEAN (1): initialization in progress
 *   CX_CLEAN    (2): matches saved copy; no save needed on context switch
 *   CX_DIRTY    (3): modified; must be saved on context switch
 * For stateless CXUs CX_READ_STATUS() returns 0xffffffff (no state to manage).
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/ci.h"
#include "../include/utils.h"
#include "../zoo/mulacc/mulacc.h"
#include "../zoo/mulacc/mulacc_common.h"
#include "../zoo/muldiv/muldiv.h"
#include "../zoo/muldiv/muldiv_common.h"
#include "../zoo/addsub/addsub.h"
#include "../zoo/addsub/addsub_common.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */

static int pass_count = 0;
static int fail_count = 0;

#define CHECK(label, expr) do { \
    int _v = (expr); \
    if (_v) { printf("  [PASS] %s\n", label); pass_count++; } \
    else    { printf("  [FAIL] %s\n", label); fail_count++; } \
} while(0)

#define CHECK_EQ(label, a, b) do { \
    int32_t _a = (a), _b = (b); \
    if (_a == _b) { printf("  [PASS] %s  (%d == %d)\n", label, _a, _b); pass_count++; } \
    else          { printf("  [FAIL] %s  (got %d, expected %d)\n", label, _a, _b); fail_count++; } \
} while(0)

static const char *dc_name(uint dc) {
    switch (dc) {
        case CX_OFF:      return "OFF";
        case CX_PRECLEAN: return "PRECLEAN";
        case CX_CLEAN:    return "CLEAN";
        case CX_DIRTY:    return "DIRTY";
        default:          return "???";
    }
}

static void dump_csrs(const char *tag) {
    cx_sel_t cx_idx          = cx_csr_read(CX_INDEX);
    cx_selidx_t mcx_sel      = { .idx = cx_csr_read(MCX_SELECTOR) };
    cx_status_t cx_stat      = { .idx = cx_csr_read(CX_STATUS) };
    VLOG("  [CSR %s]\n", tag);
    VLOG("    CX_INDEX    (0x%03x): 0x%08x  (%d)\n", CX_INDEX, (uint)cx_idx, cx_idx);
    VLOG("    MCX_SELECTOR(0x%03x): 0x%08x  cx_id=%u state_id=%u cxe=%u version=%u\n",
           MCX_SELECTOR, mcx_sel.idx,
           mcx_sel.sel.cx_id, mcx_sel.sel.state_id,
           mcx_sel.sel.cxe,   mcx_sel.sel.version);
    VLOG("    CX_STATUS   (0x%03x): 0x%08x  IV=%u IC=%u IS=%u OF=%u IF=%u OP=%u CU=%u\n",
           CX_STATUS, cx_stat.idx,
           cx_stat.sel.IV, cx_stat.sel.IC, cx_stat.sel.IS,
           cx_stat.sel.OF, cx_stat.sel.IF, cx_stat.sel.OP, cx_stat.sel.CU);
}

static void dump_status_after_sel(cx_sel_t sel, const char *name) {
    cx_sel(sel);
    cx_sel_t cx_idx          = cx_csr_read(CX_INDEX);
    cx_selidx_t mcx_sel      = { .idx = cx_csr_read(MCX_SELECTOR) };
    cx_stctxs_t hw_status    = { .idx = CX_READ_STATUS() };
    VLOG("  [after cx_sel(%s=%d)]\n", name, sel);
    VLOG("    CX_INDEX     = %d  (== sel? %s)\n", cx_idx, cx_idx == sel ? "YES" : "NO");
    VLOG("    MCX_SELECTOR = 0x%08x  cx_id=%u state_id=%u cxe=%u version=%u\n",
           mcx_sel.idx,
           mcx_sel.sel.cx_id, mcx_sel.sel.state_id,
           mcx_sel.sel.cxe,   mcx_sel.sel.version);
    if (hw_status.idx == 0xffffffff) {
        VLOG("    CX_STATUS(hw)= 0xffffffff  [stateless CXU: no state context]\n");
    } else {
        VLOG("    CX_STATUS(hw)= 0x%08x  state_size=%u dc=%s(%u)\n",
               hw_status.idx, hw_status.sel.state_size,
               dc_name(hw_status.sel.dc), hw_status.sel.dc);
    }
}

/* ------------------------------------------------------------------ */
/* Section 1: Initial CSR state                                        */
/* ------------------------------------------------------------------ */
static void test_initial_csr(void) {
    VLOG("\n=== Section 1: Initial CSR state ===\n");
    dump_csrs("initial");
    cx_sel_t cx_idx      = cx_csr_read(CX_INDEX);
    cx_error_t cx_error  = cx_error_read();
    cx_selidx_t mcx_sel  = { .idx = cx_csr_read(MCX_SELECTOR) };
    CHECK_EQ("CX_INDEX == 0 (legacy/off) at startup", cx_idx, 0);
    CHECK_EQ("CX_STATUS == 0 (no errors) at startup", (int32_t)cx_error, 0);
    /* MCX_SELECTOR is not guaranteed to be 0 on non-first-boot; its contents
     * are irrelevant when CX_INDEX==0 (LEGACY mode). Printing for info. */
    VLOG("  [INFO] MCX_SELECTOR = 0x%08x (may be non-zero from previous run; "
           "CX_INDEX=0 keeps LEGACY mode active regardless)\n", mcx_sel.idx);
}

/* ------------------------------------------------------------------ */
/* Section 2: cx_open / cx_close for every CXU                        */
/* ------------------------------------------------------------------ */
static void test_open_close_all(void) {
    VLOG("\n=== Section 2: cx_open/cx_close for all CXUs ===\n");

    struct { cx_guid_t guid; const char *name; } cxus[] = {
        { CX_GUID_MULDIV,  "muldiv"  },
        { CX_GUID_ADDSUB,  "addsub"  },
        { CX_GUID_MULACC,  "mulacc"  },
        { -1,              "invalid" },
    };

    for (int i = 0; i < 4; i++) {
        VLOG("\n  -- cx_open(%s guid=%d, CX_NO_VIRT) --\n",
               cxus[i].name, cxus[i].guid);

        cx_sel_t cx_idx_before = cx_csr_read(CX_INDEX);
        cx_sel_t sel           = cx_open(cxus[i].guid, CX_NO_VIRT, -1);
        cx_sel_t cx_idx_after  = cx_csr_read(CX_INDEX);

        VLOG("  cx_open returned: %d\n", sel);

        if (cxus[i].guid == -1) {
            CHECK("invalid GUID returns -1", sel == -1);
        } else {
            CHECK("valid GUID returns > 0", sel > 0);
            CHECK_EQ("cx_open does not change CX_INDEX",
                     cx_idx_after, cx_idx_before);

            if (sel > 0) {
                dump_status_after_sel(sel, cxus[i].name);
                cx_sel(CX_LEGACY);
                cx_close(sel);
                VLOG("  cx_close(%d) done\n", sel);
                CHECK_EQ("CX_INDEX back to 0 after cx_sel(LEGACY)",
                         (cx_sel_t)cx_csr_read(CX_INDEX), 0);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* Section 3: Stateless CXUs — operations, div/0 diagnostic, refcount */
/* ------------------------------------------------------------------ */
static void test_stateless(void) {
    VLOG("\n=== Section 3: Stateless CXUs (addsub, muldiv) ===\n");
    int result;

    /* addsub */
    VLOG("\n  -- addsub --\n");
    cx_sel_t as1 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    cx_sel_t as2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    VLOG("  cx_open #1 = %d,  cx_open #2 = %d\n", as1, as2);
    CHECK("addsub open #1 > 0", as1 > 0);
    CHECK("addsub open #2 > 0", as2 > 0);
    CHECK_EQ("stateless: both opens return same table index", as1, as2);

    if (as1 > 0) {
        cx_sel(as1);
        result = add(10, 3);     CHECK_EQ("add(10,3) == 13",      result, 13);
        result = sub(10, 3);     CHECK_EQ("sub(10,3) == 7",       result, 7);
        result = add_1000(5, 2); CHECK_EQ("add_1000(5,2) == 1007", result, 1007);
        cx_sel(CX_LEGACY);
    }
    /* Balance: two cx_open calls need two cx_close calls (refcount) */
    cx_close(as1);
    cx_close(as2);

    /* muldiv */
    VLOG("\n  -- muldiv --\n");
    cx_sel_t md1 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    cx_sel_t md2 = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    VLOG("  cx_open #1 = %d,  cx_open #2 = %d\n", md1, md2);
    CHECK("muldiv open #1 > 0", md1 > 0);
    CHECK("muldiv open #2 > 0", md2 > 0);
    CHECK_EQ("stateless: both opens return same table index", md1, md2);

    if (md1 > 0) {
        cx_sel(md1);
        result = mul(6, 7);   CHECK_EQ("mul(6,7) == 42",  result, 42);
        result = mul(0, 99);  CHECK_EQ("mul(0,99) == 0",  result, 0);
        result = div_(20, 4); CHECK_EQ("div(20,4) == 5",  result, 5);
        result = div_(7, 2);  CHECK_EQ("div(7,2) == 3",   result, 3);  /* integer div */

        /* Divide by zero: SKIPPED.
         * The RISC-V ISA specifies that integer div-by-zero returns -1 silently
         * (no trap).  However, QEMU's muldiv CXU implementation uses a bare C '/'
         * internally; on the x86 host this raises SIGFPE and crashes the VM.
         * TODO: fix the muldiv QEMU device to guard against b==0. */
        VLOG("\n  -- div/0 diagnostic: SKIPPED (known QEMU muldiv bug: SIGFPE on host) --\n");

        /* -- Error read idempotency --
         * Attempt to generate an error via overflow (mul(0x7fffffff, 2)), then
         * for each of the three error-reading methods read it twice and verify
         * both reads return the same value (idempotent).
         * NOTE: the current muldiv CXU does not set the OF error bit on integer
         * overflow — all reads return 0.  The idempotency tests pass vacuously.
         * We are NOT testing fence/clear ordering — no current CXU function is
         * slow enough for a fence to be observable. */
        VLOG("\n  -- error read idempotency (mul overflow) --\n");

        /* Method 1: cx_error_read() */
        cx_error_clear();
        (void)mul(0x7fffffff, 2);               /* overflow → sets error bits */
        cx_error_t err_A = cx_error_read();
        cx_error_t err_B = cx_error_read();
        VLOG("  cx_error_read():        A=0x%08x  B=0x%08x\n", err_A, err_B);
        CHECK_EQ("cx_error_read() idempotent", (int32_t)err_A, (int32_t)err_B);

        /* Method 2: cx_csr_read(CX_STATUS) */
        cx_error_clear();
        (void)mul(0x7fffffff, 2);
        uint err_C = cx_csr_read(CX_STATUS);
        uint err_D = cx_csr_read(CX_STATUS);
        VLOG("  cx_csr_read(CX_STATUS): C=0x%08x  D=0x%08x\n", err_C, err_D);
        CHECK_EQ("cx_csr_read(CX_STATUS) idempotent", (int32_t)err_C, (int32_t)err_D);

        /* Method 3: CX_READ_STATUS() — reads the hw context status word (cx_stctxs_t),
         * a different register from CX_STATUS; for a stateless CXU this will be
         * 0xffffffff both times. */
        cx_error_clear();
        (void)mul(0x7fffffff, 2);
        uint err_E = CX_READ_STATUS();
        uint err_F = CX_READ_STATUS();
        VLOG("  CX_READ_STATUS():       E=0x%08x  F=0x%08x\n", err_E, err_F);
        CHECK_EQ("CX_READ_STATUS() idempotent", (int32_t)err_E, (int32_t)err_F);

        cx_error_clear();
        cx_sel(CX_LEGACY);
    }
    /* Balance: two cx_open calls need two cx_close calls */
    cx_close(md1);
    cx_close(md2);
}

/* ------------------------------------------------------------------ */
/* Section 4: CX_NO_VIRT — isolation, exhaustion, INTRA_VIRT fallback */
/* ------------------------------------------------------------------ */
static void test_no_virt_isolation(void) {
    VLOG("\n=== Section 4: CX_NO_VIRT state isolation + exhaustion (mulacc) ===\n");
    VLOG("  mulacc: %d state(s), guid=%d\n",
           CX_MULACC_NUM_STATES, CX_GUID_MULACC);

    cx_sel_t selA  = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t selB  = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    VLOG("  selA=%d  selB=%d\n", selA, selB);
    CHECK("selA > 0", selA > 0);
    CHECK("selB > 0", selB > 0);
    CHECK("selA != selB (different table indices)", selA != selB);

    dump_status_after_sel(selA, "selA");
    dump_status_after_sel(selB, "selB");
    cx_sel(CX_LEGACY);

    int result;

    cx_sel(selA);
    result = reset();
    result = mac(3, 3);  CHECK_EQ("selA: mac(3,3)=9",  result, 9);
    result = mac(2, 2);  CHECK_EQ("selA: mac(2,2)=13", result, 13);

    cx_sel(selB);
    result = reset();
    result = mac(5, 5);  CHECK_EQ("selB: mac(5,5)=25 (independent from selA)", result, 25);

    cx_sel(selA);
    result = mac(1, 1);  CHECK_EQ("selA: mac(1,1)=14 (state preserved across selB use)", result, 14);

    cx_sel(selA);
    result = reset();    CHECK_EQ("selA: reset() == 0", result, 0);
    cx_sel(selB);
    result = read_acc(); CHECK_EQ("selB: read_acc()=25 (unaffected by selA reset)", result, 25);

    /* -- Exhaustion test --
     * Both states are now in use (selA=state0 acc=0, selB=state1 acc=25).
     * A 3rd CX_NO_VIRT request must fail. */
    VLOG("\n  -- Exhaustion: 3rd CX_NO_VIRT when both states are taken --\n");
    cx_sel_t selC_nv = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    VLOG("  3rd CX_NO_VIRT returned: %d  (expected -1)\n", selC_nv);
    CHECK("3rd CX_NO_VIRT returns -1 when states exhausted", selC_nv == -1);

    /* -- INTRA_VIRT as software fallback --
     * With both hardware states occupied, CX_INTRA_VIRT should succeed by
     * mapping to the LRU state slot (software spill/fill).
     * The new handle gets its own fresh logical context (acc=0). */
    VLOG("\n  -- INTRA_VIRT fallback when NO_VIRT is exhausted --\n");
    cx_sel_t selC_iv = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    VLOG("  CX_INTRA_VIRT fallback returned: %d  (expected > 0)\n", selC_iv);
    CHECK("CX_INTRA_VIRT succeeds via LRU spill when NO_VIRT exhausted", selC_iv > 0);
    if (selC_iv > 0) {
        dump_status_after_sel(selC_iv, "selC_iv");
        cx_sel(selC_iv);
        result = read_acc();
        VLOG("  INTRA_VIRT spilled handle acc = %d  (expect 0: fresh logical ctx)\n", result);
        CHECK_EQ("INTRA_VIRT fallback: fresh independent acc == 0", result, 0);
        /* Verify selA and selB were preserved by the spill/fill mechanism */
        cx_sel(selA);
        result = read_acc();
        CHECK_EQ("selA preserved after INTRA_VIRT spill: acc == 0", result, 0);
        cx_sel(selB);
        result = read_acc();
        CHECK_EQ("selB preserved after INTRA_VIRT spill: acc == 25", result, 25);
        cx_sel(CX_LEGACY);
        cx_close(selC_iv);
    }

    cx_sel(CX_LEGACY);
    cx_close(selA);
    cx_close(selB);
}

/* ------------------------------------------------------------------ */
/* Section 5: CX_INTRA_VIRT                                           */
/*   Sub-test A: two -1 opens, each gets its own state (free slots)   */
/*   Sub-test B: 3rd -1 open when full → LRU spill, fresh logical ctx */
/*   Sub-test C: explicit sharing via share_sel argument              */
/* ------------------------------------------------------------------ */
static void test_intra_virt(void) {
    VLOG("\n=== Section 5: CX_INTRA_VIRT (mulacc) ===\n");
    VLOG("  mulacc has %d hardware state slot(s).\n", CX_MULACC_NUM_STATES);
    VLOG("  INTRA_VIRT semantics:\n");
    VLOG("    share_sel=-1 : allocate a free slot, or spill LRU slot (own fresh context)\n");
    VLOG("    share_sel=N  : share the SAME physical state as selector N\n");
    int result;

    /* ------------------------------------------------------------------
     * Sub-test A: two -1 opens → each gets a free hardware state slot
     * ------------------------------------------------------------------ */
    VLOG("\n  -- Sub-test A: two fresh INTRA_VIRT opens (share_sel=-1) --\n");
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    VLOG("  selA=%d  selB=%d\n", selA, selB);
    CHECK("selA > 0", selA > 0);
    CHECK("selB > 0", selB > 0);
    CHECK("selA != selB (different table indices)", selA != selB);

    dump_status_after_sel(selA, "selA");
    dump_status_after_sel(selB, "selB");
    cx_sel(CX_LEGACY);

    cx_sel(selA); reset();
    cx_sel(selB); reset();

    cx_sel(selA);
    result = mac(4, 4);  CHECK_EQ("selA: mac(4,4)=16", result, 16);
    result = mac(1, 1);  CHECK_EQ("selA: mac(1,1)=17", result, 17);

    cx_sel(selB);
    result = mac(3, 3);  CHECK_EQ("selB: mac(3,3)=9",  result, 9);

    /* At this point: selA acc=17 (used 2 ops), selB acc=9 (used 1 op).
     * LRU = selB (fewest uses). */

    /* ------------------------------------------------------------------
     * Sub-test B: 3rd -1 open when both slots are full → LRU spill
     *
     * The OS maps selC to the LRU slot (selB's, state_id=1).  selB's
     * current state (acc=9) is saved to memory.  selC gets a fresh,
     * independently initialised logical context (acc=0).
     *
     * Key distinction: selC does NOT inherit selB's value — it has its
     * OWN fresh context.  After switching back to selB the OS restores
     * the saved value (acc=9).
     * ------------------------------------------------------------------ */
    VLOG("\n  -- Sub-test B: 3rd open (share_sel=-1) when slots full → LRU spill --\n");
    VLOG("  selA acc=17 (2 ops), selB acc=9 (1 op) → LRU slot = selB's\n");
    cx_sel_t selC = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    VLOG("  selC=%d\n", selC);
    CHECK("selC > 0 (INTRA_VIRT spill succeeded)", selC > 0);
    if (selC > 0) {
        dump_status_after_sel(selC, "selC");
        cx_sel(selC);
        result = read_acc();
        VLOG("  selC read_acc() = %d  (expect 0: fresh independent context)\n", result);
        CHECK_EQ("selC: fresh logical context, acc == 0", result, 0);
        result = mac(5, 5);
        CHECK_EQ("selC: mac(5,5) = 25", result, 25);

        /* Spill/fill: switching back must restore preserved states */
        VLOG("  Switching back to selA and selB to verify spill/fill...\n");
        cx_sel(selA);
        result = read_acc();
        CHECK_EQ("selA preserved after selC spill: acc == 17", result, 17);
        cx_sel(selB);
        result = read_acc();
        CHECK_EQ("selB restored by spill/fill: acc == 9", result, 9);

        cx_close(selC);
    }

    /* ------------------------------------------------------------------
     * Sub-test C: explicit sharing using share_sel argument
     *
     * cx_open(guid, CX_INTRA_VIRT, selA) returns a new selector that
     * maps to the SAME physical state slot as selA.  Both selectors
     * see each other's writes immediately (no save/restore between them).
     * ------------------------------------------------------------------ */
    VLOG("\n  -- Sub-test C: explicit sharing via share_sel=selA --\n");
    VLOG("  selA currently has acc=17\n");
    cx_sel_t selD = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, selA);
    VLOG("  selD = cx_open(mulacc, CX_INTRA_VIRT, selA=%d) → %d\n", selA, selD);
    CHECK("selD > 0 (sharing with selA)", selD > 0);
    if (selD > 0) {
        cx_sel(selD);
        result = read_acc();
        VLOG("  selD read_acc() = %d  (expect 17: shares selA's state)\n", result);
        CHECK_EQ("selD shares selA's state: acc == 17", result, 17);

        result = mac(1, 1);
        CHECK_EQ("selD: mac(1,1) = 18 (selA acc was 17)", result, 18);
        cx_sel(selA);
        result = read_acc();
        CHECK_EQ("selA sees selD's write: acc == 18", result, 18);

        cx_sel(selB);
        result = read_acc();
        CHECK_EQ("selB unaffected by selA/selD sharing: acc == 9", result, 9);

        cx_close(selD);
    }

    cx_sel(CX_LEGACY);
    cx_close(selA);
    cx_close(selB);
}

/* ------------------------------------------------------------------ */
/* Section 6: State reset on close + re-open; refcount                */
/* ------------------------------------------------------------------ */
static void test_state_reset_on_reopen(void) {
    VLOG("\n=== Section 6: State reset on close + re-open; refcount ===\n");
    int result;

    /* -- Basic state reset -- */
    VLOG("\n  -- State reset after close + re-open --\n");
    cx_sel_t sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    CHECK("open returns > 0", sel > 0);
    if (sel > 0) {
        cx_sel(sel);
        reset();
        result = mac(7, 7);  CHECK_EQ("mac(7,7)=49 before close", result, 49);
        result = mac(1, 1);  CHECK_EQ("mac(1,1)=50 before close", result, 50);
        cx_sel(CX_LEGACY);
        cx_close(sel);
        VLOG("  Closed sel=%d. Re-opening...\n", sel);

        cx_sel_t sel2 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
        VLOG("  new sel=%d\n", sel2);
        CHECK("re-open returns > 0", sel2 > 0);
        if (sel2 > 0) {
            cx_sel(sel2);
            result = read_acc(); CHECK_EQ("accumulator reset to 0 after re-open", result, 0);
            result = mac(2, 3);  CHECK_EQ("mac(2,3)=6 on fresh state", result, 6);
            cx_sel(CX_LEGACY);
            cx_close(sel2);
        }
    }

    /* -- Refcount test (stateless CXU: addsub) --
     *
     * Refcount is about sharing the same actual state slot.
     * cx_open with share_sel=-1 always allocates a fresh independent handle.
     * cx_open with share_sel=N returns a new handle sharing N's slot;
     * the slot is freed only when ALL shared handles are closed.
     *
     * Part 1: fresh opens (share_sel=-1) → distinct selectors each time.
     * Part 2: explicit sharing (share_sel=N) → same selector returned;
     *         close 3x, kernel "Freeing" should appear only after the last. */
    VLOG("\n  -- Refcount test: stateless CXU (addsub) --\n");

    /* Part 1: fresh opens — current spec: stateless CXUs return the same
     * table index for all opens regardless of share_sel (no per-open state
     * to distinguish).  NOTE: future spec revisions may change this so that
     * fresh opens return distinct selectors even for stateless CXUs.
     * Refcount is tracked internally; the slot is freed only after the
     * matching number of cx_close calls. */
    cx_sel_t f1 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    cx_sel_t f2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    cx_sel_t f3 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    VLOG("  fresh opens (share_sel=-1): f1=%d  f2=%d  f3=%d\n", f1, f2, f3);
    CHECK("f1 > 0", f1 > 0);
    CHECK_EQ("stateless: all fresh opens return same index (f1==f2) [NOTE: may change in future spec]", f1, f2);
    CHECK_EQ("stateless: all fresh opens return same index (f2==f3) [NOTE: may change in future spec]", f2, f3);
    if (f1 > 0) cx_close(f1);
    if (f2 > 0) cx_close(f2);
    if (f3 > 0) cx_close(f3);

    /* Part 2: shared opens increment refcount on same slot */
    cx_sel_t r1 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    cx_sel_t r2 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, r1);
    cx_sel_t r3 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, r2);
    VLOG("  shared opens: r1=%d  r2=%d  r3=%d\n", r1, r2, r3);
    CHECK("r1 > 0", r1 > 0);
    CHECK_EQ("shared open returns same selector (r1==r2)", r1, r2);
    CHECK_EQ("shared open returns same selector (r2==r3)", r2, r3);

    if (r1 > 0) {
        cx_sel(r1);
        result = add(2, 3);
        CHECK_EQ("add(2,3)==5 (3 refs open)", result, 5);
        cx_sel(CX_LEGACY);
    }
    cx_close(r1);
    VLOG("  Closed 1x.\n");
    dump_csrs("after closing ADDSUB r1");
    cx_close(r2);
    VLOG("  Closed 2x.\n");
    dump_csrs("after closing ADDSUB r2");
    cx_close(r3);
    dump_csrs("after closing ADDSUB r3");
    VLOG("  Closed 3x.  'Freeing cx_index' should appear only after the 3rd close.\n");

    /* -- Refcount test (INTRA_VIRT sharing: mulacc) --
     *
     * cx_open(guid, CX_INTRA_VIRT, s1) returns a new selector that shares
     * s1's physical state slot.  Each cx_open adds a reference; the slot
     * is freed only when all handles are closed.
     *
     * Verified by: accumulating via the original selector, then reading
     * back via the shared handles. */
    VLOG("\n  -- Refcount test: INTRA_VIRT sharing open x3 / close x3 --\n");
    cx_sel_t s1 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    CHECK("s1 > 0 (base handle)", s1 > 0);
    if (s1 > 0) {
        cx_sel(s1); reset();
        result = mac(3, 3);
        CHECK_EQ("s1: mac(3,3) = 9", result, 9);

        cx_sel_t s2 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, s1);
        cx_sel_t s3 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, s1);
        VLOG("  s1=%d  s2=%d  s3=%d\n", s1, s2, s3);
        CHECK("s2 > 0 (shares s1)", s2 > 0);
        CHECK("s3 > 0 (shares s1)", s3 > 0);

        if (s2 > 0) {
            cx_sel(s2);
            result = read_acc();
            CHECK_EQ("s2 sees s1's acc == 9", result, 9);
        }
        if (s3 > 0) {
            cx_sel(s3);
            result = read_acc();
            CHECK_EQ("s3 sees s1's acc == 9", result, 9);
        }
        cx_sel(CX_LEGACY);
        if (s3 > 0) {
            cx_close(s3);
            VLOG("INTRA_VIRT s3 closing\n");
            cx_sel(s3); //if they are the same, this should be selecting s1
            result = read_acc();
            CHECK_EQ("reselecting s3, same as s1, that's stil open, so acc == 9", result, 9);
            if (result == 9) {
                cx_close(s3);
            }
        } else {
            CHECK_EQ("INTRA_VIRT s3 invalid.", 0, 1); //add to error count
        } 
                if (s2 > 0) {
            cx_close(s2);
            VLOG("INTRA_VIRT s2 closing\n");
            cx_sel(s2); //if they are the same, this should be selecting s1
            result = read_acc();
            CHECK_EQ("reselecting s2, same as s1, that's stil open, so acc == 9", result, 9);
            if (result == 9) {
                cx_close(s3);
            }
        } else {
            CHECK_EQ("INTRA_VIRT s2 invalid.", 0, 1); //add to error count
        } 
        if (s1 > 0) {
            cx_close(s1);
            VLOG("INTRA_VIRT s1 closing\n");
            cx_sel(s1); //if they are the same, this should be selecting s1
            result = read_acc();
            CHECK("reselecting s1, shouldn't work. acc != 9", result != 9); // there's better ways to test this
            if (result == 9) {
                cx_close(s1);
            }
        } else {
            CHECK_EQ("INTRA_VIRT s1 invalid.", 0, 1); //add to error count
        }
    }
}

/* ------------------------------------------------------------------ */
/* Section 7: CX_INDEX tracks cx_sel() correctly                      */
/* ------------------------------------------------------------------ */
static void test_cx_index_tracking(void) {
    VLOG("\n=== Section 7: CX_INDEX/MCX_SELECTOR tracking ===\n");

    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t selS = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    VLOG("  selA(mulacc)=%d  selB(mulacc)=%d  selS(addsub)=%d\n",
           selA, selB, selS);

    cx_sel(selA);
    cx_sel_t cx_idx      = cx_csr_read(CX_INDEX);
    cx_selidx_t mcx_sel  = { .idx = cx_csr_read(MCX_SELECTOR) };
    VLOG("  after cx_sel(selA=%d): CX_INDEX=%d MCX_SELECTOR=0x%08x\n",
           selA, cx_idx, mcx_sel.idx);
    CHECK_EQ("CX_INDEX == selA after cx_sel(selA)", cx_idx, selA);
    VLOG("    MCX_SELECTOR decoded: cx_id=%u state_id=%u cxe=%u version=%u\n",
           mcx_sel.sel.cx_id, mcx_sel.sel.state_id,
           mcx_sel.sel.cxe,   mcx_sel.sel.version);

    cx_sel(selB);
    cx_idx  = cx_csr_read(CX_INDEX);
    mcx_sel = (cx_selidx_t){ .idx = cx_csr_read(MCX_SELECTOR) };
    VLOG("  after cx_sel(selB=%d): CX_INDEX=%d MCX_SELECTOR=0x%08x\n",
           selB, cx_idx, mcx_sel.idx);
    CHECK_EQ("CX_INDEX == selB after cx_sel(selB)", cx_idx, selB);
    VLOG("    MCX_SELECTOR decoded: cx_id=%u state_id=%u cxe=%u version=%u\n",
           mcx_sel.sel.cx_id, mcx_sel.sel.state_id,
           mcx_sel.sel.cxe,   mcx_sel.sel.version);

    cx_sel(selS);
    cx_idx  = cx_csr_read(CX_INDEX);
    mcx_sel = (cx_selidx_t){ .idx = cx_csr_read(MCX_SELECTOR) };
    VLOG("  after cx_sel(selS=%d): CX_INDEX=%d MCX_SELECTOR=0x%08x\n",
           selS, cx_idx, mcx_sel.idx);
    CHECK_EQ("CX_INDEX == selS after cx_sel(selS)", cx_idx, selS);
    VLOG("    MCX_SELECTOR decoded: cx_id=%u state_id=%u cxe=%u version=%u\n",
           mcx_sel.sel.cx_id, mcx_sel.sel.state_id,
           mcx_sel.sel.cxe,   mcx_sel.sel.version);

    cx_sel(CX_LEGACY);
    cx_idx  = cx_csr_read(CX_INDEX);
    mcx_sel = (cx_selidx_t){ .idx = cx_csr_read(MCX_SELECTOR) };
    VLOG("  after cx_sel(LEGACY=0):\n");
    VLOG("    CX_INDEX     = %d\n", cx_idx);
    VLOG("    MCX_SELECTOR = 0x%08x  cx_id=%u state_id=%u cxe=%u version=%u\n",
           mcx_sel.idx,
           mcx_sel.sel.cx_id, mcx_sel.sel.state_id,
           mcx_sel.sel.cxe,   mcx_sel.sel.version);
    CHECK_EQ("CX_INDEX == 0 after cx_sel(LEGACY)", cx_idx, 0);
    CHECK_EQ("MCX_SELECTOR.idx == 0 after cx_sel(LEGACY)", (int32_t)mcx_sel.idx, 0);
    CHECK_EQ("MCX_SELECTOR.version == 0 (LEGACY active)", (int32_t)mcx_sel.sel.version, 0);

    cx_close(selA);
    cx_close(selB);
    cx_close(selS);
}

/* ------------------------------------------------------------------ */
/* Section 8: cx_open does not disturb active CX_INDEX                */
/* ------------------------------------------------------------------ */
static void test_open_preserves_index(void) {
    VLOG("\n=== Section 8: cx_open preserves active CX_INDEX ===\n");

    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    CHECK("selA > 0", selA > 0);
    if (selA <= 0) return;

    cx_sel(selA);
    uint cx_idx_before = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX before 2nd cx_open: %u\n", cx_idx_before);

    cx_sel_t selB  = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    uint cx_idx_after = cx_csr_read(CX_INDEX);
    VLOG("  CX_INDEX after cx_open(selB): %u  selB=%d\n", cx_idx_after, selB);
    CHECK_EQ("cx_open does not change active CX_INDEX",
             (int32_t)cx_idx_after, (int32_t)cx_idx_before);

    cx_sel(CX_LEGACY);
    cx_close(selA);
    if (selB > 0) cx_close(selB);
}

/* ------------------------------------------------------------------ */
/* Section 9: Error cases                                              */
/* ------------------------------------------------------------------ */
static void test_errors(void) {
    VLOG("\n=== Section 9: Error cases ===\n");
    cx_sel_t sel;

    sel = cx_open(-1, CX_NO_VIRT, -1);
    VLOG("  cx_open(guid=-1):   %d\n", sel);
    CHECK("invalid GUID -1 returns -1", sel == -1);

    sel = cx_open(9999, CX_NO_VIRT, -1);
    VLOG("  cx_open(guid=9999): %d\n", sel);
    CHECK("unknown GUID 9999 returns -1", sel == -1);

    /* Exhaust all mulacc states (NUM_STATES=2), then try CX_NO_VIRT again */
    VLOG("\n  Exhausting all %d mulacc CX_NO_VIRT states...\n",
           CX_MULACC_NUM_STATES);
    cx_sel_t slots[8];
    int n = 0;
    for (int i = 0; i < CX_MULACC_NUM_STATES; i++) {
        slots[n] = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
        VLOG("    cx_open #%d = %d\n", i+1, slots[n]);
        if (slots[n] > 0) n++;
    }
    sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    VLOG("  cx_open after exhaustion: %d\n", sel);
    CHECK("CX_NO_VIRT fails when all states taken", sel == -1);

    for (int i = 0; i < n; i++) cx_close(slots[i]);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    printf("========================================\n");
    printf("  CX PROBE & DIAGNOSTIC TEST\n");
    printf("  mulacc guid=%d states=%d\n", CX_GUID_MULACC, CX_MULACC_NUM_STATES);
    printf("  addsub guid=%d states=%d\n", CX_GUID_ADDSUB, CX_ADDSUB_NUM_STATES);
    printf("  muldiv guid=%d states=%d\n", CX_GUID_MULDIV, CX_MULDIV_NUM_STATES);
    printf("========================================\n");

    cx_sel(CX_LEGACY);

    test_initial_csr();
    test_open_close_all();
    test_stateless();
    test_no_virt_isolation();
    test_intra_virt();
    test_state_reset_on_reopen();
    test_cx_index_tracking();
    test_open_preserves_index();
    test_errors();

    printf("\n========================================\n");
    printf("  RESULTS: %d passed, %d failed\n", pass_count, fail_count);
    printf("========================================\n");

    return fail_count ? 1 : 0;
}
