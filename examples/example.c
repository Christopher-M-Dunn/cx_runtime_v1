#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../include/ci.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)
#include "../zoo/addsub/addsub.h"
#include "../zoo/muldiv/muldiv.h"

cx_sel_t test_cx_open( cx_guid_t my_guid, cx_share_t my_share )
{
    cx_sel_t my_sel = cx_open( my_guid, my_share, -1 );
    if( my_sel >= 0 ) return my_sel;

    fprintf( stderr, "error: cx_open guid %08x share %d\n", my_guid, my_share );
    exit( -1 );
}

void cx_error_test( cx_sel_t cx_sel ) {
    cx_error_t cx_error = cx_error_read();
    if (cx_error > 0) {
        fprintf( stderr, "error: cx_error %08x with cx_sel %d\n", cx_error, cx_sel );
        exit( -1 );
    }
}

void my_cx_test() 
{
    int32_t a = 3;
    int32_t b = 5;
    
    int32_t result = 0;

    cx_share_t my_shareA=0, my_shareB=0;

    cx_sel_t my_selA = test_cx_open( CX_GUID_ADDSUB, my_shareA );
    cx_sel_t my_selB = test_cx_open( CX_GUID_MULDIV, my_shareB );

    printf("cx_sel_AS: %d, cx_sel_MD: %d\n", my_selA, my_selB);

    cx_error_clear();
    cx_sel( my_selA ); // ABI rule 4
    result = add( a, b ); // ABI rule 3
    cx_error_test( my_selA );
    printf( "result add: %d\n", result );

    cx_error_clear();
    cx_sel( my_selB );
    result = mul( a, b );
    cx_error_test( my_selB );
    printf( "result mul: %d\n", result );

    cx_error_clear();
    cx_sel( my_selA );
    result = sub( a, b );
    printf( "result sub: %d\n", result );
    result = add_1000( a, b );
    cx_error_test( my_selA );
    printf( "result add_1000: %d\n", result );

    cx_close( my_selB );
    cx_close( my_selA );

    cx_sel( CX_LEGACY ); // ABI rule 5
}


int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    cx_sel( CX_LEGACY ); // ABI rule 2
    my_cx_test();
    return 0;
}