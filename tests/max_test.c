#include <stdio.h>
#include <stdlib.h>

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

static __attribute__ ((noinline)) int scalar_max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    int a = (rand() % 10) - 5;
    int b = (rand() % 10) - 5;
    int c = scalar_max(a, b);
    printf("res: %d\n", c);
    return 0;
}
