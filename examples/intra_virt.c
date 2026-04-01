#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

int a = 5, b = 3, c = 2, res = 0;

void intra_open_1() {
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    assert(selA > 0);
    assert(selB > 0);
    cx_close(selA);
    cx_close(selB);
}

void intra_open_2() {
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selC = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selD = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);

    assert(selA > 0);
    assert(selB > 0);
    assert(selC > 0);
    assert(selD > 0);
    cx_close(selA);
    cx_close(selB);
    cx_close(selC);
    cx_close(selD);
}

void intra_open_3() {
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selC = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selD = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);

    assert(selA > 0);
    assert(selB > 0);
    assert(selC > 0);
    assert(selD > 0);

    assert(GET_CX_ID(selA) == GET_CX_ID(selB));
    assert(GET_CX_ID(selB) == GET_CX_ID(selC));
    assert(GET_CX_ID(selC) == GET_CX_ID(selD));

    cx_sel(selB);
    res = mac(a, a);
    assert(res == 25);

    cx_sel(selC);
    res = mac(b, b);
    assert(res == 9);

    cx_sel(selD);
    res = mac(a, b);
    assert(res == 15);
    
    cx_sel(selB);
    res = mac(b, b);
    assert(res == 34);

    cx_sel(selC);
    res = mac(b, b);
    assert(res == 18);

    cx_sel(selD);
    res = mac(b, a);
    assert(res == 30);

    cx_sel(selB);
    res = mac(b, b);
    assert(res == 43);

    cx_close(selA);
    cx_close(selB);
    cx_close(selC);
    cx_close(selD);
}

void intra_open_4() {
    // cx_sel( CX_LEGACY );
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    assert(selA > 0);
    assert(selB > 0);

    cx_sel(selA);
    res = mac(a, a);
    assert(res == 25);

    cx_sel_t selC = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    // assert(CX_READ_STATE(0) == 25);

    cx_sel(selC);
    assert(CX_READ_STATE(0) == 0);

    cx_close(selA);
    cx_close(selB);
    cx_close(selC);
}

// Changing the initialization to system land can break this test.
void intra_open_5() {
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    assert(selB > 0);

    cx_sel(selB);
    res = mac(a, a);
    assert(res == 25);

    cx_sel_t selC = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    assert(CX_READ_STATE(0) == 25);
    assert(selC > 0);
    assert(selB == cx_csr_read(MCX_SELECTOR));

    res = mac(a, b);
    assert(res == 40);

    cx_sel(selC);
    assert(CX_READ_STATE(0) == 0);
    res = mac(a, b);
    assert(res == 15);

    cx_close(selA);
    cx_close(selB);
    cx_close(selC);

    cx_sel( CX_LEGACY );
}

void intra_open_6() {
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, selA);
    cx_sel_t selC = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selD = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, selC);

    assert(selA > 0);
    assert(selB > 0);
    assert(selC > 0);
    assert(selD > 0);

    assert(GET_CX_ID(selA) == GET_CX_ID(selB));
    assert(GET_CX_ID(selB) == GET_CX_ID(selC));
    assert(GET_CX_ID(selC) == GET_CX_ID(selD));

    assert(GET_CX_STATE(selA) == GET_CX_STATE(selB));
    assert(GET_CX_STATE(selB) != GET_CX_STATE(selC));
    assert(GET_CX_STATE(selC) == GET_CX_STATE(selD));

    cx_sel(selA);
    res = mac(a, a);
    assert(res == 25);

    cx_sel(selB);
    res = mac(a, b);
    assert(res == 15);

    cx_sel(selA);
    res = mac(c, c);
    assert(res == 29);

    cx_sel(selC);
    res = mac(a, c);
    assert(res == 10);

    cx_sel(selD);
    res = mac(b, c);
    assert(res == 6);

    cx_sel(selC);
    res = mac(c, c);
    assert(res == 14);
    res = mac(a, a);
    assert(res == 39);

    cx_sel(selA);
    res = mac(b, b);
    assert(res == 38);

    cx_sel(selC);
    res = mac(a, a);
    assert(res == 64);

    cx_sel(selB);
    res = mac(b, b);
    assert(res == 24);

    cx_close(selA);
    cx_close(selB);
    cx_close(selC);
    cx_close(selD);
    cx_sel( CX_LEGACY );
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    intra_open_1();
    intra_open_2();
    intra_open_3();
    intra_open_4();
    intra_open_5();
    intra_open_6();
    printf("intra virt test passed!\n");
    return 0;
}
