#ifndef UTILS_H
#define UTILS_H

#define MCX_SELECTOR  0x012 // should be 0xBC0
#define CX_INDEX      0x011 // should be 0x800
#define CX_STATUS     0x801 // should be 0x801, was using 0x013
#define MCX_TABLE     0x145 // should be 0xBC1

#define __CX__         //(FIXME: compiler builtin)
#define CX_LEGACY 0

#define CUSTOM0 0xb
#define CUSTOM1 0x2b
#define CUSTOM2 0x5b

#define CX_REG_TYPE  CUSTOM0
#define CX_IMM_TYPE  CUSTOM1
#define CX_FLEX_TYPE CUSTOM2

#define CX_INVALID_SELECTOR 0x80000000
#define CX_LEGACY 0
#define MCX_VERSION 1
#define CX_SEL_TABLE_NUM_ENTRIES 1024
// #define DEBUG 0
#define UNASSIGNED_STATE -1

#define MAX_CF_IDS 1024
#define MAX_CX_ID 255
#define NUM_CX 8

/* =========  CX function return sentinels  ==================================
 *
 * CXU functions return int32_t. These sentinel values occupy the extreme ends
 * of the int32_t range and are reserved for signalling flags back to the
 * runtime (pending a proper solution where env is passed into CXU functions).
 * A truly invalid function (NULL function pointer) returns -1 and sets IF flag.
 * All below sentinel values will be converted to a user returned value of -1
 * unless otherwise stated.
 *
 * Positive sentinels (INT32_MAX = 0x7FFFFFFF downward):
 *   OP_POS             — OP flag; Arithmetic error or overflow, returns INTMAX
 *   IF_INVALID_RET_0   — IF flag; Invalid function, but handled by the CXU
 *                        Returns 0.
 *   CU_CUSTOM_BIT_0..15 — CU flag; Sets the Nth bit of cxs_error CSR
 *
 * Negative Sentinels (INT32_MIN = 0x80000000 upward):
 *   OP_NEG             — OP flag; Arithmetic error or underflow, returns INTMIN
 *   OP_DOMAIN          — request operand(s) or state are a domain error for the
 *                        custom function or custom CSR access. Returns -1
 *   CU_CUSTOM_BIT_16..31 — CU flag; Sets the Nth bit of cxs_error CSR
 *   IF_INVALID_RET_NEG1 — Invalid function, but handled by the CXU
 *
 * ========================================================================= */

#define FUNC_SENTINEL_OP_POS              ((int32_t) 0x7FFFFFFF)  /* INT32_MAX     */
#define FUNC_SENTINEL_IF_INVALID_RET_0    ((int32_t) 0x7FFFFFFE)  /* INT32_MAX - 1 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_0     ((int32_t) 0x7FFFFFFD)  /* INT32_MAX - 2 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_1     ((int32_t) 0x7FFFFFFC)  /* INT32_MAX - 3 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_2     ((int32_t) 0x7FFFFFFB)  /* INT32_MAX - 4 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_3     ((int32_t) 0x7FFFFFFA)  /* INT32_MAX - 5 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_4     ((int32_t) 0x7FFFFFF9)  /* INT32_MAX - 6 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_5     ((int32_t) 0x7FFFFFF8)  /* INT32_MAX - 7 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_6     ((int32_t) 0x7FFFFFF7)  /* INT32_MAX - 8 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_7     ((int32_t) 0x7FFFFFF6)  /* INT32_MAX - 9 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_8     ((int32_t) 0x7FFFFFF5)  /* INT32_MAX - 10 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_9     ((int32_t) 0x7FFFFFF4)  /* INT32_MAX - 11 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_10    ((int32_t) 0x7FFFFFF3)  /* INT32_MAX - 12 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_11    ((int32_t) 0x7FFFFFF2)  /* INT32_MAX - 13 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_12    ((int32_t) 0x7FFFFFF1)  /* INT32_MAX - 14 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_13    ((int32_t) 0x7FFFFFF0)  /* INT32_MAX - 15 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_14    ((int32_t) 0x7FFFFFEF)  /* INT32_MAX - 16 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_15    ((int32_t) 0x7FFFFFEE)  /* INT32_MAX - 17 */

#define FUNC_SENTINEL_OP_NEG              ((int32_t) 0x80000000)  /* INT32_MIN      */
#define FUNC_SENTINEL_OP_DOMAIN           ((int32_t) 0x80000001)  /* INT32_MIN + 1  */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_16    ((int32_t) 0x80000002)  /* INT32_MIN + 2  */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_17    ((int32_t) 0x80000003)  /* INT32_MIN + 3  */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_18    ((int32_t) 0x80000004)  /* INT32_MIN + 4  */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_19    ((int32_t) 0x80000005)  /* INT32_MIN + 5  */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_20    ((int32_t) 0x80000006)  /* INT32_MIN + 6  */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_21    ((int32_t) 0x80000007)  /* INT32_MIN + 7  */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_22    ((int32_t) 0x80000008)  /* INT32_MIN + 8  */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_23    ((int32_t) 0x80000009)  /* INT32_MIN + 9  */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_24    ((int32_t) 0x8000000A)  /* INT32_MIN + 10 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_25    ((int32_t) 0x8000000B)  /* INT32_MIN + 11 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_26    ((int32_t) 0x8000000C)  /* INT32_MIN + 12 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_27    ((int32_t) 0x8000000D)  /* INT32_MIN + 13 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_28    ((int32_t) 0x8000000E)  /* INT32_MIN + 14 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_29    ((int32_t) 0x8000000F)  /* INT32_MIN + 15 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_30    ((int32_t) 0x80000010)  /* INT32_MIN + 16 */
#define FUNC_SENTINEL_CU_CUSTOM_BIT_31    ((int32_t) 0x80000011)  /* INT32_MIN + 17 */
#define FUNC_SENTINEL_IF_INVALID_RET_NEG1 ((int32_t) 0x80000012)  /* INT32_MIN + 18 */

// number of words in a state
// for space reasons (laziness), this is not the proper size
// because none of the current cxus use more space.
// this should be 2^15
#define MAX_STATE_SIZE 1024 

/* cx_selector CSR */
#define CX_ID_START_INDEX 0
#define CX_ID_BITS 8

#define CX_STATE_START_INDEX 16
#define CX_STATE_ID_BITS 8

#define CX_VERSION_START_INDEX 28
#define CX_VERSION_ID_BITS 3

#define CX_CXE_START_INDEX 31
#define CX_CXE_BITS 1

/* cx_context_status_word CSR */
#define CX_STATUS_START_INDEX 0
#define CX_STATUS_BITS 2

#define CX_STATE_SIZE_START_INDEX 0
#define CX_STATE_SIZE_BITS 16

#define CX_DATA_CLEAN_START_INDEX 26
#define CX_DATA_CLEAN_BITS 2

#define CX_STATUS_VERSION_START_INDEX 28

#define CX_RESET_START_INDEX 31
#define CX_RESET_BITS 1

#define CX_HW_INIT 0
#define CX_OS_INIT 1

/* cx_status CSR */
#define CX_IV_START_INDEX 0
#define CX_IV_BITS 1

#define CX_IC_START_INDEX 1
#define CX_IC_BITS 1

#define CX_IS_START_INDEX 2
#define CX_IS_BITS 1

#define CX_OF_START_INDEX 3
#define CX_OF_BITS 1

#define CX_IF_START_INDEX 4
#define CX_IF_BITS 1

#define CX_OP_START_INDEX 5
#define CX_OP_BITS 1

#define CX_CU_START_INDEX 6
#define CX_CU_BITS 1

/* cx sel user type */
#define CX_INDEX_START_INDEX 0
#define CX_INDEX_START_BITS 10

#define CX_INDEX_VALID_INDEX 31
#define CX_INDEX_VALID_BITS 1

/* cx share type */
#define CX_SHARE_START_INDEX 0
#define CX_SHARE_BITS 2

// ========= cx helpers ===========

#define GET_BITS(cx_sel, start_bit, n) \
    ((cx_sel >> start_bit) & (((1 << n) - 1) ))

// ========= cx selector helpers ===========

#define GET_CX_ID(cx_sel) \
    GET_BITS(cx_sel, CX_ID_START_INDEX, CX_ID_BITS)

#define GET_CX_STATE(cx_sel) \
    GET_BITS(cx_sel, CX_STATE_START_INDEX, CX_STATE_ID_BITS)

#define GET_CX_CXE(cx_sel) \
    GET_BITS(cx_sel, CX_CXE_START_INDEX, CX_CXE_BITS)

#define GET_CX_VERSION(cx_sel) \
    GET_BITS(cx_sel, CX_VERSION_START_INDEX, CX_VERSION_ID_BITS)

// ========= cx state context status helpers ===========

#define GET_CX_STATUS_VERSION(cx_sel) \
    GET_BITS(cx_sel, CX_STATUS_VERSION_START_INDEX, CX_VERSION_ID_BITS)

#define GET_CX_DATA_CLEAN(cx_sel) \
    GET_BITS(cx_sel, CX_DATA_CLEAN_START_INDEX, CX_DATA_CLEAN_BITS)

#define GET_CX_STATE_SIZE(cx_sel) \
    GET_BITS(cx_sel, CX_STATE_SIZE_START_INDEX, CX_STATE_SIZE_BITS)

#define GET_CX_RESET(cx_sel) \
    GET_BITS(cx_sel, CX_RESET_START_INDEX, CX_RESET_BITS)

// ========= cx user selector helpers ===========

#define GET_INDEX(cx_sel) \
    GET_BITS(cx_sel, CX_INDEX_START_INDEX, CX_INDEX_BITS)

#define GET_INDEX_VALID(cx_sel) \
    GET_BITS(cx_sel, CX_INDEX_VALID_INDEX, CX_INDEX_VALID_BITS)

// ========= cx status helpers ===========



// ========= cx share type helpers ==========
#define GET_SHARE_TYPE(cx_share) \
    GET_BITS(cx_share, CX_SHARE_START_INDEX, CX_SHARE_BITS) 

// other

#define MAX_CXU_ID 1 << CX_ID_BITS
#define MAX_STATE_ID 1 << CX_STATE_ID_BITS

typedef unsigned int uint;

typedef union {
     struct {
        uint state_size  : CX_STATE_SIZE_BITS;
        uint reserved0   : 10;
        uint dc          : CX_DATA_CLEAN_BITS;
        uint version     : CX_VERSION_ID_BITS;
        uint R           : CX_RESET_BITS;
     } sel;
      uint idx;
} cx_stctxs_t;

typedef union {
    struct {
        uint cx_id     : CX_ID_BITS;
        uint reserved1 : 8;
        uint state_id  : CX_STATE_ID_BITS;
        uint reserved0 : 4;
        uint version   : CX_VERSION_ID_BITS;
        uint cxe       : CX_CXE_BITS;
    } sel;
        uint idx;
 } cx_selidx_t;

typedef union {
    struct {
        uint IV        : CX_IV_BITS;
        uint IC        : CX_IC_BITS;
        uint IS        : CX_IS_BITS;
        uint OF        : CX_OF_BITS;
        uint IF        : CX_IF_BITS;
        uint OP        : CX_OP_BITS;
        uint CU        : CX_CU_BITS;
        uint reserved0 : 25;
    } sel;
    int idx;
} cx_status_t;

typedef union {
    struct {
        uint idx       : CX_INDEX_START_BITS;
        uint reserved  : 21;
        uint iv        : CX_INDEX_VALID_BITS;
    } sel;
        uint idx;
 } cx_sel_user_t;

enum CX_CS {
    CX_OFF, 
    CX_PRECLEAN, 
    CX_CLEAN,
    CX_DIRTY
};

typedef enum {
	CX_NO_VIRT,
	CX_INTRA_VIRT,
	CX_INTER_VIRT,
    CX_FULL_VIRT
} CX_SHARE_T;

#define CX_REG_HELPER(cf_id, rs1, rs2)      ({           \
    register int __v;                                    \
    asm volatile("      cx_reg " #cf_id ",%0,%1,%2;\n\t" \
                 : "=r" (__v)                            \
                 : "r" (rs1), "r" (rs2)                  \
                 : "memory"                              \
    );                                                   \
	__v;							                     \
    })

#define __ASM_STR(x)	#x
#define cx_csr_read(csr)				                \
({								                        \
	register unsigned int __v;				            \
	__asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)	\
			      : "=r" (__v) :			            \
			      : "memory");			                \
	__v;							                    \
})

#define cx_csr_write(csr, val)					        \
({								                        \
	unsigned int __v = (unsigned int)(val);		        \
	__asm__ __volatile__ ("csrw " __ASM_STR(csr) ", %0"	\
			      : : "rK" (__v)			            \
			      : "memory");			                \
})

#define CX_WRITE_STATE(index, value)  ({                   \
    asm volatile("      cx_reg 1020, zero,%0,%1;\n\t"      \
                 :                                         \
                 : "r" (index), "r" (value)                \
                 :                                         \
    );   })

#define CX_WRITE_STATUS(status)  ({                        \
    asm volatile("      cx_reg 1022, zero,%0,x0;\n\t"      \
                 :                                         \
                 : "r" (status)                            \
                 :                                         \
    );   })

#define CX_READ_STATUS()                CX_REG_HELPER(1023, 0, 0)
// #define CX_WRITE_STATUS(status)             CX_REG_HELPER(1022, 0, status, 0)
#define CX_READ_STATE(index)            CX_REG_HELPER(1021, index, 0)
// #define CX_WRITE_STATE(index, value)        CX_REG_HELPER(1020, 0, index, value)

#endif