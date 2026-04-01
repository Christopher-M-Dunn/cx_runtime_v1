#define _GNU_SOURCE
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)


int a = 5, b = 3, res = 0;

pthread_mutex_t lock;

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

void* cx_mac_thread_1(void *ptr) {
    int val = *(int *)ptr;
    mac(val, val);
    return NULL;
}

typedef struct mac_worker_t {
    cx_sel_t sel;
    int val;
    int N;
} mac_worker_t;

void* cx_mac_thread_2(void *args) {
    mac_worker_t *t = (mac_worker_t *)args;
    cx_sel(t->sel);
    int val = t->val;
    int N = t->N;
    for (int i = 0; i < N; i++) {
        mac(val, val);
    }

    // TODO: There needs to be a fencing instruction, or else doing this read
    // will be done while cx instructions are still in-flight.

    // pthread_mutex_lock(&lock);
    // int temp = CX_READ_STATE(0);
    // printf("sum: %d\n", temp);
    // res += temp;
    // pthread_mutex_unlock(&lock);
    return NULL;
}

void multi_thread_1() {
    int N = 10;
    pthread_t tid[ N ];
    void *ret;
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    assert(selA > 0);
    assert(selB > 0);
    cx_sel(selB);

    int *ptr = malloc(sizeof(int));
    *ptr = a;
    for (int i = 0; i < N; i++) {
        assert( pthread_create(&tid[i], NULL, cx_mac_thread_1, ptr) == 0 );
    }

    for (int i = 0; i < N; i++) {
        assert( pthread_join(tid[i], &ret) == 0 );
    }

    int res = CX_READ_STATE(0);
    assert( res == N * a * a);
    cx_close(selA);
    cx_close(selB);
    free(ret);
}

void multi_thread_2() {
    int N = 10;
    res = 0;
    pthread_t tid[ N ];
    void *ret;
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t *sels = malloc(sizeof(cx_sel_t) * N);
    mac_worker_t **args = malloc(sizeof(mac_worker_t *) * N);
    for (int i = 0; i < N; i++) {
        sels[i] = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
        args[i] = malloc(sizeof(mac_worker_t));
        args[i]->N = N;
        args[i]->sel = sels[i];
        args[i]->val = i;
    }

    if (pthread_mutex_init(&lock, NULL) != 0) { 
        printf("\n mutex init has failed\n"); 
        return; 
    }

    for (int i = 0; i < N; i++) {
        assert( pthread_create(&tid[i], NULL, cx_mac_thread_2, (void *)args[i]) == 0 );
    }

    for (int i = 0; i < N; i++) {
        assert( pthread_join(tid[i], &ret) == 0 );
    }

    pthread_mutex_destroy(&lock);

    for (int i = 0; i < N; i++) {
        cx_sel(args[i]->sel);
        res += CX_READ_STATE(0);
    }

    int scalar_sum = 0;
    for (int i = 0; i < N; i++) {
        scalar_sum += i * i;
    }

    scalar_sum *= N;
    // printf("res: %d, scalar_sum: %d\n", res, scalar_sum);
    assert( res == scalar_sum);
    for (int i = 0; i < N; i++) {
        cx_close(sels[i]);
    }
    cx_close(selA);
    free(ret);
    free(sels);
    for (int i = 0; i < N; i++) {
        free(args[i]);
    }
    free(args);
}

// void intra_open_1() {
//     cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
//     cx_sel_t selB = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
//     assert(selA > 0);
//     assert(selB > 0);


//     cx_close(selA);
//     cx_close(selB);
// }

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    for (int i = 0; i < 100; i++) {
        multi_thread_1();
        multi_thread_2();
    }
    printf("Threaded test passed!\n");
    return 0;
}