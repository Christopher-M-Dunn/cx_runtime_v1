#include "exports.h"

// structs populated by spike / qemu

int32_t (*cx_func_error[]) (int32_t, int32_t, cx_selidx_t) = {0};

cx_func_stub_t cx_funcs[MAX_CX_ID] = {
    cx_func_muldiv,
    cx_func_addsub,
    cx_func_mulacc, 
    cx_func_pext,
    cx_func_vector,
    cx_func_vector,
    cx_func_max,
    cx_func_nn_acc
    };

int32_t num_cfs[MAX_CX_ID] = {
    CX_MULDIV_NUM_FUNCS, 
    CX_ADDSUB_NUM_FUNCS,
    CX_MULACC_NUM_FUNCS, 
    CX_PEXT_NUM_FUNCS,
    CX_VECTOR_NUM_FUNCS,
    CX_VECTOR_NUM_FUNCS,
    CX_MAX_NUM_FUNCS,
    CX_NN_ACC_NUM_FUNCS
    };

int32_t num_states[MAX_STATE_SIZE] = {
    CX_MULDIV_NUM_STATES,
    CX_ADDSUB_NUM_STATES,
    CX_MULACC_NUM_STATES,
    CX_PEXT_NUM_STATES,
    CX_VECTOR_NUM_STATES,
    CX_VECTOR_NUM_STATES,
    CX_MAX_NUM_STATES,
    CX_NN_ACC_NUM_STATES
};

// Fill unused functions in their arrays error
void cx_init_funcs() {
    // VULNERABILITY: no validation that registered CXUs comply with expected
    // table structure. Stateful CXUs currently must implement system CFs
    // 1020-1023 (write_state, read_state, write_status, read_status) because
    // the kernel calls them directly during cx_open/initialize_state. CXUs
    // with unsized function arrays (e.g. addsub, muldiv) expose raw memory
    // past their last entry when accessed by CF ID >= num_cfs. The new spec
    // will remove the system CF requirement; until then, any CXU author who
    // omits system CFs on a stateful CXU will silently call a garbage pointer.
    // New spec will deal with this differently so for now all CXs are expected
    // to comply with standard.
    init_cx_func_mulacc();
    init_cx_func_vector();
    init_cx_func_nn_acc();

    for (int i = NUM_CX; i < MAX_CX_ID; i++) {
        cx_funcs[i] = cx_func_error;
        num_cfs[i] = 0;
        num_states[i] = 0;
    }
}