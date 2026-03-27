#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/ci.h"
#include "../zoo/addsub/addsub.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

static const int a = 3, b = 5;

void basic_stateless_test() {
    int result;

    cx_error_t cx_error;

    int cx_sel_A0 = cx_open(CX_GUID_ADDSUB, -1, -1);

    /* Index 0 should be reserved */
    assert( cx_sel_A0 > 0 );

    cx_error_clear();
    cx_sel(cx_sel_A0);

    result = add(a, b);
    assert( result == 8 );
    cx_error = cx_error_read();

    cx_close(cx_sel_A0);

    cx_sel_t cx_sel_A1 = cx_open(CX_GUID_ADDSUB, -1, -1);
    cx_sel_t cx_sel_A2 = cx_open(CX_GUID_ADDSUB, -1, -1);

    // The same index should be returned for stateless cxus
    assert( cx_sel_A1 > 0 );
    assert( cx_sel_A1 == cx_sel_A2 );
    cx_close( cx_sel_A1 );
    cx_close( cx_sel_A2 );
    cx_sel( CX_LEGACY );
}

void stateless_fork() {
    int cx_sel_A0 = cx_open(CX_GUID_ADDSUB, -1, -1);
    assert( cx_sel_A0 > 0 );
    cx_sel(cx_sel_A0);

    pid_t pid = fork();
    assert( pid >= 0 );
    if (pid == 0) {
        assert(sub(a, b) == a - b);
        int cx_sel_A1 = cx_open(CX_GUID_ADDSUB, -1, -1);
        assert( cx_sel_A1 > 0 );
        assert( cx_sel_A1 == cx_sel_A0 );
        cx_close(cx_sel_A0);
        cx_close(cx_sel_A1);
        exit(EXIT_SUCCESS);
    } else {
        sleep(1);
        assert(add(a, a) == a + a);
        wait(NULL);
    }
    assert(add(a, b) == a + b);
    cx_close(cx_sel_A0);
    int cx_sel_A2 = cx_open(CX_GUID_ADDSUB, -1, -1);
    assert( cx_sel_A2 > 0 );
    assert( cx_sel_A0 != cx_sel_A2 );
    cx_close(cx_sel_A2);
    
    cx_sel( CX_LEGACY );
}

void stateless_fork_multiple_opens() {
    int cx_sel_A0 = cx_open(CX_GUID_ADDSUB, -1, -1);
    cx_open(CX_GUID_ADDSUB, -1, -1);
    cx_open(CX_GUID_ADDSUB, -1, -1);
    assert( cx_sel_A0 > 0 );
    cx_sel(cx_sel_A0);

    pid_t pid = fork();
    assert( pid >= 0 );
    if (pid == 0) {
        assert(sub(a, b) == a - b);
        cx_close(cx_sel_A0);
        cx_close(cx_sel_A0);
        cx_close(cx_sel_A0);
        exit(EXIT_SUCCESS);
    } else {
        sleep(1);
        assert(add(a, a) == a + a);
        wait(NULL);
    }
    assert(add(a, b) == a + b);
    cx_close(cx_sel_A0);
    cx_close(cx_sel_A0);
    cx_close(cx_sel_A0);
    
    cx_sel( CX_LEGACY );
}

int main() {

    cx_sel( CX_LEGACY );
    basic_stateless_test();
    stateless_fork();
    stateless_fork_multiple_opens();
    printf("stateless test passed\n");

    return 0;
}
