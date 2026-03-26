#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

static const int a = 5, b = 3;

void* test_thread() {
    return NULL;
}

void basic_thread_test() {
    pthread_t thid;
    void *ret;

    if (pthread_create(&thid, NULL, test_thread, NULL) != 0) {
        perror("pthread_create() error");
        exit(1);
    }

    if (pthread_join(thid, &ret) != 0) {
        perror("pthread_create() error");
        exit(3);
    }
}

void* cx_mac_thread(void *ptr) {
    int val = *(int *)ptr;
    mac(val, val);
    return NULL;
}

void cx_open_across_threads() {
    int N = 10;
    pthread_t tid[ N ];
    void *ret;
    cx_sel_t cx_sel_A0 = cx_open(CX_GUID_MULACC, CX_FULL_VIRT, -1);
    assert(cx_sel_A0 > 0);
    cx_sel(cx_sel_A0);

    int *ptr = malloc(sizeof(int));
    *ptr = a;

    for (int i = 0; i < N; i++) {
        assert( pthread_create(&tid[i], NULL, cx_mac_thread, ptr) == 0 );
    }

    for (int i = 0; i < N; i++) {
        assert( pthread_join(tid[i], &ret) == 0 );
    }

    int res = CX_READ_STATE(0);
    assert( res == N * a * a);
    cx_close(cx_sel_A0);

    return;
}

int main() {
    basic_thread_test();
    cx_open_across_threads();
    printf("Thread test complete\n");
    return 0;
}