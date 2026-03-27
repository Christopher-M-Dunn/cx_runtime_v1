#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"
#include "../zoo/addsub/addsub.h"
#include "../zoo/muldiv/muldiv.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <unistd.h> 

static int a = 3, b = 5, c = 2, result = 0;


void basic_no_virt() {
    int cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_A0 > 0 );
    int cx_sel_A1 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_A1 > 0 );
    int cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert( cx_sel_A2 == -1 );
    cx_close(cx_sel_A0);

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, 1);
    assert( cx_sel_A0 > 0 );
    cx_close(cx_sel_A0);

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, 1023);
    assert( cx_sel_A0 > 0 );
    cx_close(cx_sel_A0);

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, 1024);
    assert( cx_sel_A0 == -1 );
    cx_close(cx_sel_A0);

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -2);
    assert( cx_sel_A0 == -1 );
    cx_close(cx_sel_A0);

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, 0);
    assert( cx_sel_A0 == -1 );
    cx_close(cx_sel_A0);

    cx_close(cx_sel_A1);

    pid_t pid = fork();
    assert( pid >= 0);
    if (pid == 0) {
        cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
        assert( cx_sel_A0 > 0 );
        sleep(1);
        cx_close(cx_sel_A0);
        exit(EXIT_SUCCESS);
    } else {
        cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
        assert( cx_sel_A0 > 0 );
        wait(NULL);
    }
    cx_close(cx_sel_A0);

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert (cx_sel_A0 > 0);

    // TODO: Need a way to check if the mcx_selector values are different 
    // across both processes
    pid = fork();
    assert( pid >= 0);
    if (pid == 0) {
        cx_sel(cx_sel_A0);
        mac(a, a);
        assert(cx_error_read() == 0);
        assert(CX_READ_STATE(0) == a * a);
        sleep(1);
        cx_close(cx_sel_A0);
        exit(EXIT_SUCCESS);
    } else {
        cx_sel(cx_sel_A0);
        mac(b, b);
        assert(cx_error_read() == 0);
        assert(CX_READ_STATE(0) == b * b);
        wait(NULL);
    }
    cx_close(cx_sel_A0);
}

void basic_intra_virt() {

    int cx_sel_A = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, 1);
    assert( cx_sel_A == -1 );

    cx_sel_A = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, 1023);
    assert( cx_sel_A == -1 );

    cx_sel_A = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, 1024);
    assert( cx_sel_A == -1 );

    cx_sel_A = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, 0);
    assert( cx_sel_A == -1 );
    
    int cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    assert( cx_sel_A0 > 0 );
    cx_sel(cx_sel_A0);
    uint mcx_selector_A0 = cx_csr_read(MCX_SELECTOR);
    int cx_sel_A1 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    assert( cx_sel_A1 > 0 );
    cx_sel(cx_sel_A1);
    uint mcx_selector_A1 = cx_csr_read(MCX_SELECTOR);

    // Each cx_open should get an exclusive state until no more 
    // exclusive states are available; then, it will start sharing
    // with states opened with the corresponding share type.
    assert( mcx_selector_A0 != mcx_selector_A1 );
    assert( cx_sel_A0 != cx_sel_A1 );

    int cx_sel_A3 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, cx_sel_A0);
    int cx_sel_A4 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, cx_sel_A3);
    cx_sel(cx_sel_A3);
    mcx_selector_A0 = cx_csr_read(MCX_SELECTOR);
    cx_sel(cx_sel_A0);
    mcx_selector_A1 = cx_csr_read(MCX_SELECTOR);
    assert(mcx_selector_A0 == mcx_selector_A1);

    assert( cx_sel_A3 > 0 );
    assert( cx_sel_A4 > 0 );

    int cx_sel_A5 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, 1022);
    int cx_sel_A6 = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, 1025);

    assert( cx_sel_A5 == -1 );
    assert( cx_sel_A6 == -1 );

    cx_close(cx_sel_A0);
    cx_close(cx_sel_A1);
    cx_close(cx_sel_A3);
    cx_close(cx_sel_A4);
}

void basic_inter_virt() {
    int cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, 0);
    assert( cx_sel_A0 == -1 );

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, 1);
    assert( cx_sel_A0 == -1 );

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, 1023);
    assert( cx_sel_A0 == -1 );

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, 1024);
    assert( cx_sel_A0 == -1 );

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
    assert( cx_sel_A0 > 0 );

    int cx_sel_A1 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
    assert( cx_sel_A1 > 0 );
    assert( cx_sel_A0 != cx_sel_A1 );
    
    // can't share interprocess
    int cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
    assert( cx_sel_A2 == -1 );

    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) { 
        cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
        assert( cx_sel_A2 > 0 );
        int cx_sel_A3 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
        assert( cx_sel_A3 > 0 );

        cx_close( cx_sel_A0 );
        cx_close( cx_sel_A1 );
        cx_close( cx_sel_A2 );
        cx_close( cx_sel_A3 );

        exit(EXIT_SUCCESS);
    } else {
        cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
        assert( cx_sel_A2 == -1 );
        wait(NULL);
    }

    cx_close( cx_sel_A0 );
    cx_close( cx_sel_A1 );
}

void basic_full_virt() {
    int cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, 0);
    assert( cx_sel_A0 == -1 );

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, 1);
    assert( cx_sel_A0 == -1 );

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, 1023);
    assert( cx_sel_A0 == -1 );

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, 1024);
    assert( cx_sel_A0 == -1 );

    cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, -1);
    assert( cx_sel_A0 > 0 );

    int cx_sel_A1 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, -1);
    assert( cx_sel_A1 > 0 );

    cx_sel(cx_sel_A0);
    uint mcx_selector_A0 = cx_csr_read(MCX_SELECTOR);
    cx_sel(cx_sel_A1);
    uint mcx_selector_A1 = cx_csr_read(MCX_SELECTOR);
    assert( mcx_selector_A0 != mcx_selector_A1 );

    int cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, cx_sel_A1);
    assert( cx_sel_A2 > 0 );
    cx_sel(cx_sel_A2);
    uint mcx_selector_A2 = cx_csr_read(MCX_SELECTOR);
    assert( mcx_selector_A1 == mcx_selector_A2 );

    cx_close(cx_sel_A2);

    pid_t pid = fork();
    assert(pid >= 0);
    if (pid == 0) {
        //child
        cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, cx_sel_A0);
        assert( cx_sel_A2 > 0 );
        int cx_sel_A3 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, -1);
        assert( cx_sel_A3 > 0 );

        cx_close( cx_sel_A0 );
        cx_close( cx_sel_A1 );
        cx_close( cx_sel_A2 );
        cx_close( cx_sel_A3 );

        exit(EXIT_SUCCESS);
    } else {
        cx_sel_A2 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, -1);
        assert( cx_sel_A2 == -1 );
        wait(NULL);
    }

    cx_close( cx_sel_A0 );
    cx_close( cx_sel_A1 );

}

int main() {
    basic_no_virt();
    basic_intra_virt();
    basic_inter_virt();
    printf("Finished virtualization types test\n");
    return 0;
}