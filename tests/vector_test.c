#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/ci.h"
#include "../zoo/vector/vector.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <unistd.h>

#define A 0
#define B 1
#define C 2
#define D 3

static int vec[3][CX_VECTOR_NUM_REGS][CX_VECTOR_VL];

static void reset_vec_reg(int state_id, int reg) {
    for (int i = 0; i < CX_VECTOR_VL; i++) {
        vec[state_id][reg][i] = 0;
    }
}

static void init_vec(int state_id) {
    for (int i = 0; i < CX_VECTOR_NUM_REGS; i++) {
        reset_vec_reg(state_id, i);
    }
}

static void check_vector_reg_valid(int state_id, int reg) {
    int reg_start_idx = CX_VECTOR_VL * reg;
    for (int i = 0; i < CX_VECTOR_VL; i++) {
        // printf("expected: %d, actual: %d\n", vec[state_id][reg][i], CX_READ_STATE(reg_start_idx + i));
        assert(CX_READ_STATE(reg_start_idx + i) == vec[state_id][reg][i]);
    }
}

static void check_vector_regs_valid(int state_id) {
    for (int i = 0; i < CX_VECTOR_NUM_REGS; i++) {
        check_vector_reg_valid(state_id, i);
    }
}

static inline void write_vec_idx(int state_id, int reg, int idx, int v) {
    vec[state_id][reg][idx] = v;
}

static void write_vec_reg_inc(int state_id, int reg) {
    for (int i = 0; i < CX_VECTOR_VL; i++) {
        write_vec_idx(state_id, reg, i, i);
    }
}

static void write_vec_reg_2(int state_id, int reg) {
    for (int i = 0; i < CX_VECTOR_VL; i++) {
        write_vec_idx(state_id, reg, i, 2);
    }
}

static void vector_add(int state_id, int src, int dest) {
    for (int i = 0; i < CX_VECTOR_VL; i++) {
        vec[state_id][src][i] += vec[state_id][dest][i];
    }
}

int basic_test() {
    int result;

    cx_sel(CX_LEGACY);
    int cx_sel_C0 = cx_open(CX_GUID_VECTOR, CX_NO_VIRT, -1);
    int cx_sel_C1 = cx_open(CX_GUID_VECTOR, CX_NO_VIRT, -1);

    assert ( cx_sel_C0 != -1 );
    assert ( cx_sel_C1 != -1 );

    // printf("cx_sel_0: %08x, cx_sel_1: %08x\n", cx_sel_C0, cx_sel_C1);

    cx_sel(cx_sel_C0);

    set_inc(A);
    set2(B);
    addv(A, B);

    init_vec(0);
    write_vec_reg_inc(0, A);
    write_vec_reg_2(0, B);
    vector_add(0, A, B);
    
    check_vector_regs_valid(0);

    cx_sel(cx_sel_C1);
    set_inc(A);
    set_inc(B);
    addv(A, B);

    write_vec_reg_inc(1, A);
    write_vec_reg_inc(1, B);
    vector_add(1, A, B);
    check_vector_regs_valid(1);

    cx_sel(cx_sel_C0);
    check_vector_regs_valid(0);

    cx_sel(cx_sel_C1);
    check_vector_regs_valid(1);
    
    cx_close(cx_sel_C1);
    cx_close(cx_sel_C0);
    cx_sel(CX_LEGACY);
}

void basic_intra() {
    int temp0 = cx_open(CX_GUID_VECTOR, CX_NO_VIRT, -1);
    int temp1 = cx_open(CX_GUID_VECTOR, CX_NO_VIRT, -1);
    int temp2 = cx_open(CX_GUID_VECTOR, CX_NO_VIRT, -1);
    int selA = cx_open(CX_GUID_VECTOR, CX_INTRA_VIRT, -1);
    int selB = cx_open(CX_GUID_VECTOR, CX_INTRA_VIRT, -1);
    int selC = cx_open(CX_GUID_VECTOR, CX_INTRA_VIRT, -1);

    assert(temp0 != -1);
    assert(temp1 != -1);
    assert(temp2 != -1);
    assert(selA != -1);
    assert(selB != -1);
    assert(selC != -1);

    cx_sel(selA);
    set_inc(A);
    set_inc(B);
    addv(A, B);

    write_vec_reg_inc(0, A);
    write_vec_reg_inc(0, B);
    vector_add(0, A, B);
    check_vector_regs_valid(0);

    cx_sel(selB);
    set2(A);
    set_inc(B);
    addv(A, B);
    write_vec_reg_2(1, A);
    write_vec_reg_inc(1, B);
    vector_add(1, A, B);
    check_vector_regs_valid(1);

    cx_sel(selC);
    set2(A);
    set2(B);
    addv(A, B);
    write_vec_reg_2(2, A);
    write_vec_reg_2(2, B);
    vector_add(2, A, B);
    check_vector_regs_valid(2);

    cx_sel(selA);
    check_vector_regs_valid(0);

    cx_sel(selB);
    check_vector_regs_valid(1);

    cx_sel(selC);
    check_vector_regs_valid(2);

    cx_close(selA);
    cx_close(selB);
    cx_close(selC);
    cx_close(temp0);
    cx_close(temp1);
    cx_close(temp2);

    cx_sel(CX_LEGACY);
}

void basic_inter() {
    // int temp = cx_open(CX_GUID_VECTOR, CX_NO_VIRT, -1);
    int selA = cx_open(CX_GUID_VECTOR, CX_INTER_VIRT, -1);
    assert(selA > 0);

    cx_sel(selA);

    set_inc(A);
    set2(B);
    addv(A, B);

    init_vec(0);
    write_vec_reg_inc(0, A);
    write_vec_reg_2(0, B);
    vector_add(0, A, B);
    
    check_vector_regs_valid(0);

    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) { 
        // Child

        // Was the data cloned correctly?
        check_vector_regs_valid(0);

        set2(B);
        addv(A, B);

        write_vec_reg_2(0, B);
        vector_add(0, A, B);
        
        check_vector_regs_valid(0);

        resetv();
        set_inc(A);
        set_inc(B);
        addv(A, B);
        addv(A, B);

        init_vec(0);
        write_vec_reg_inc(0, A);
        write_vec_reg_inc(0, B);
        vector_add(0, A, B);
        vector_add(0, A, B);

        check_vector_regs_valid(0);
        cx_close(selA);

        exit(EXIT_SUCCESS);
    } else {
        // Parent
        cx_sel(selA);

        // Data still correct?
        check_vector_regs_valid(0);

        set2(A);
        set2(B);
        addv(A, B);

        write_vec_reg_2(0, A);
        write_vec_reg_2(0, B);
        vector_add(0, A, B);
        
        check_vector_regs_valid(0);

        int status;
        waitpid(pid, &status, 0);
        assert(status == 0);
    }
    cx_sel(selA);
    check_vector_regs_valid(0);
    cx_close(selA);

    cx_sel( CX_LEGACY );
}

void basic_full() {
    int selA = cx_open(CX_GUID_VECTOR, CX_FULL_VIRT, -1);
    assert(selA > 0);

    cx_sel(selA);

    set_inc(A);
    set2(B);
    addv(A, B);

    init_vec(0);
    write_vec_reg_inc(0, A);
    write_vec_reg_2(0, B);
    vector_add(0, A, B);
    
    check_vector_regs_valid(0);

    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) { 
        // Child

        // Was the data cloned correctly?
        check_vector_regs_valid(0);

        set2(B);
        addv(A, B);

        write_vec_reg_2(0, B);
        vector_add(0, A, B);
        
        check_vector_regs_valid(0);

        resetv();
        set_inc(A);
        set_inc(B);
        addv(A, B);
        addv(A, B);

        init_vec(0);
        write_vec_reg_inc(0, A);
        write_vec_reg_inc(0, B);
        vector_add(0, A, B);
        vector_add(0, A, B);

        check_vector_regs_valid(0);
        cx_close(selA);

        exit(EXIT_SUCCESS);
    } else {
        // Parent
        cx_sel(selA);

        // Data still correct?
        check_vector_regs_valid(0);

        set2(A);
        set2(B);
        addv(A, B);

        write_vec_reg_2(0, A);
        write_vec_reg_2(0, B);
        vector_add(0, A, B);
        
        check_vector_regs_valid(0);

        int status;
        waitpid(pid, &status, 0);
        assert(status == 0);
    }
    cx_sel(selA);
    check_vector_regs_valid(0);
    cx_close(selA);

    cx_sel( CX_LEGACY );
}

void inter_2() {
    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        
        // We want the parent to execute and be waiting for the child, so that we 
        // can make sure the contexts are inter-virtualized.
        sleep(1);

        int selA = cx_open(CX_GUID_VECTOR, CX_INTER_VIRT, -1);
        int selB = cx_open(CX_GUID_VECTOR, CX_INTER_VIRT, -1);
        int selC = cx_open(CX_GUID_VECTOR, CX_INTER_VIRT, -1);
        int selD = cx_open(CX_GUID_VECTOR, CX_INTER_VIRT, -1);

        assert(selA > 0);
        assert(selB > 0);
        assert(selC > 0);
        assert(selD > 0);

        cx_close(selA);
        cx_close(selB);
        cx_close(selC);
        cx_close(selD);

        exit(EXIT_SUCCESS);
    } else {
        int selA = cx_open(CX_GUID_VECTOR, CX_INTER_VIRT, -1);
        int selB = cx_open(CX_GUID_VECTOR, CX_INTER_VIRT, -1);
        int selC = cx_open(CX_GUID_VECTOR, CX_INTER_VIRT, -1);
        int selD = cx_open(CX_GUID_VECTOR, CX_INTER_VIRT, -1);

        assert(selA > 0);
        assert(selB > 0);
        assert(selC > 0);
        assert(selD > 0);

        int status;
        waitpid(pid, &status, 0);
        assert(status == 0);

        cx_close(selA);
        cx_close(selB);
        cx_close(selC);
        cx_close(selD);
    }
    cx_sel( CX_LEGACY );
}

void virt_threaded() {
    
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    basic_test();
    basic_intra();
    basic_inter();
    inter_2();
    printf("completed!\n");
    return 0;
}
