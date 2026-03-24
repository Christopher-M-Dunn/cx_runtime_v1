#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../include/ci.h"
#include "../zoo/muldiv/muldiv.h"
#include "../zoo/addsub/addsub.h"

void test() {
    int a = 3, b = 5, result = 0;

    cx_sel_t selM = cx_open(CX_GUID_MULDIV, CX_NO_VIRT, -1);
    assert(selM > 0);
    cx_sel_t selA = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert(selA > 0);
    assert(selA != selM);
    cx_sel_t selA0 = cx_open(CX_GUID_ADDSUB, CX_NO_VIRT, -1);
    assert(selA0 > 0);
    assert(selA0 == selA);

    cx_sel(selA);
    printf("active sel: %08x\n", cx_csr_read(MCX_SELECTOR));
    result = add(a, b);
    printf("selA: %08x, selM: %08x, res: %d\n", selA, selM, result);
    assert(result == 8);

    cx_sel(selM);
    result = mul(a, b);
    assert(result == 15);

    cx_sel(selA);
    result = add(a, a);
    assert(result == 6);

    cx_sel(selA0);
    result = add(b, b);
    assert(result == 10);

    cx_close(selA0);
    cx_close(selA);
    cx_close(selM);

    cx_sel( CX_LEGACY );
}

int main() {
    test();
    printf("stateless test passed!\n");
    return 0;
}
