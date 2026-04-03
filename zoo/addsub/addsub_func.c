#include <stdint.h>
#include <stddef.h>
#include "../../include/utils.h"

#include "addsub_func.h"

static inline int32_t add_func(int32_t a, int32_t b, cx_selidx_t sys_sel)
{
    return a + b;
}

static inline int32_t sub_func(int32_t a, int32_t b, cx_selidx_t sys_sel)
{
    return a - b;
}

static inline int32_t add_1000(int32_t a, int32_t b, cx_selidx_t sys_sel)
{
    return a + b + 1000;
}

static int32_t cx_func_undefined(int32_t, int32_t, cx_selidx_t) { return FUNC_SENTINEL_IF_INVALID_RET_0; }
static int32_t cx_func_op_pos(int32_t, int32_t, cx_selidx_t)   { return FUNC_SENTINEL_OP_POS; }
static int32_t cx_func_cu_bit0(int32_t, int32_t, cx_selidx_t)  { return FUNC_SENTINEL_CU_CUSTOM_BIT_0; }

/* CF IDs are 0-indexed; CX_ADDSUB_NUM_FUNCS == highest CF ID + 1.
 * Unused CF IDs within the range must be NULL or cx_func_undefined. */
int32_t (*cx_func_addsub[]) (int32_t, int32_t, cx_selidx_t) = {
    add_func,          /* CF 0 */
    NULL,              /* CF 1 — unused */
    sub_func,          /* CF 2 */
    cx_func_undefined, /* CF 3 — IF sentinel (FUNC_SENTINEL_IF_INVALID_RET_0) */
    add_1000,          /* CF 4 */
    cx_func_op_pos,    /* CF 5 — OP sentinel test */
    cx_func_cu_bit0,   /* CF 6 — CU sentinel test */
};