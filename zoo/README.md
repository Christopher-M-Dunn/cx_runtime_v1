# Adding a new CXU

3 files need to be added to define a new CXU:
1. **\<CX_name\>_common.h:** Information about cx_guid, number of states, and number of cfs.
2. **\<CX_name\>_func.h:** Header file for functional implementation.
3. **\<CX_name\>_func.c:** The functional implementation of each CXU. This is the software version of what would typically be done in hardware (verilog, VHDL, etc.), and is called by QEMU when the mcx_selector is set to have this CXU active.

Additionally, 2 files can come in handy for user programs:
1. **\<CX_name\>.h:** Header file for user code to include.
2. **\<CX_name\>.c:** Implements calls to cx\_{reg, imm, flex}, with each function being given a unique cf_id.

After defining the 3 files, the \<CX_name\>_func.h header needs to be included in zoo/exports.h.

The function pointer array needs to be added to zoo/exports.c in cx_funcs. Additionally, the num_cfs and num_states should be updated as well.

**CF ID convention:** `<CX_NAME>_NUM_FUNCS` must equal the highest CF ID + 1, not the count of implemented functions. This value does not include system the CFs (1020-1023) that stateful CXUs must implement. CF IDs do not need to be contiguous, but unused slots within the range must be set to `NULL`. The runtime (cx_helper) sets the IF flag and returns -1 when it encounters a NULL slot. In simulation, CXU functions cannot set flags directly — this requires returning sentinal values or passing `env` into the function signature, which is future work.

The Makefile in the cx_runtime/ directory should be updated as well. Rules should be added to build the \<CX_name\>_func.c for qemu using the host compiler, and the \<CX_name\>.c using the RISC-V cross-compiler. mulacc can be referenced for an example on how to add a new CXU to the Makefile. 

Finally, update the NUM_CX value in include/utils.h.

### Note: Stateful functions

Stateful functions may require additional initialization (see zoo/mulacc/mulacc_func.c). This initialization function should be added to the cx_init_funcs() in zoo/exports.c. For example, if a new stateful CXU has 3 functions, an initialization function should be defined that sets all unused CFs to error (CF 3->1019), up to CF 1019. CF 1020 - 1023 should be used for accessing state / status information, as defined in section 2.6.x of the cx spec.

# Updating the Kernel:
In the cx_sys.c file in the kernel, include the \<CX_name\>_common.h header. Then, in the cx_init function, update the next free index in the cx_map with the new cx_guid and state information. 

Finally, in linux/include/linux/sched.h, update the NUM_CX definition.

The new CXU should now work in QEMU, with stateful CXUs able to context switch and virtualize states.
