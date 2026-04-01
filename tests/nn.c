#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/ci.h"
#include "../zoo/max/max.h"
#include "../zoo/mulacc/mulacc.h"
#include "../zoo/nn_acc/nn_acc.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>

#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define SIZE_0 32
#define SIZE_1 32
#define N_LAYERS 5
#define UNROLL_FACTOR 4

#define IDX(i, j, n) (i * n + j)

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
}

static inline int scalar_max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

int* alloc_mat(int m, int n) {
  int *matrix = (int *)calloc(n * m, sizeof(int));
  return matrix;
}

int *gen_mat(int m, int n) 
{
    int *matrix = alloc_mat(m, n);
    for(int i = 0; i < n; i++) {
        for(int j = 0; j < m; j++){
            matrix[IDX(i, j, n)] = (rand() % 10) - 5;
        }
    }
    return matrix;
}

void print_mat(int *A, int m, int n) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      VLOG("%d ", A[IDX(i, j, n)]);
    }
    VLOG("\n");
  }
}

void mat_check_equal(int *A, int *B, int m, int n) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      if (A[IDX(i, j, n)] != B[IDX(i, j, n)]) {
        VLOG("i: %d, j: %d, A: %d, B: %d\n", i, j, A[IDX(i, j, n)], B[IDX(i, j, n)]);
      }
      assert(A[IDX(i, j, n)] == B[IDX(i, j, n)]);
    }
  }
}

void free_mat(int *A, int n) {
  free(A);
}

void nn_layer_scalar(int* A, int* B, int* C, int m, int n) {
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      for (int k = 0; k < n; k += UNROLL_FACTOR) {
        C[IDX(i, j, n)] += A[IDX(i, k, n)] * B[IDX(k, j, n)];
        C[IDX(i, j, n)] += A[IDX(i, k + 1, n)] * B[IDX(k + 1, j, n)];
        C[IDX(i, j, n)] += A[IDX(i, k + 2, n)] * B[IDX(k + 2, j, n)];
        C[IDX(i, j, n)] += A[IDX(i, k + 3, n)] * B[IDX(k + 3, j, n)];
      }
      C[IDX(i, j, n)] = scalar_max(0, C[IDX(i, j, n)]);
    }
  }
}

void nn_layer_mac_relu(const int *A, const int *B, int *C, int n, cx_sel_t mac_sel, cx_sel_t max_sel) {
    int temp = 0, k = 0;
    cx_sel(mac_sel);
    reset();
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            k = 0;
            int acc = 0;
            for (int k = 0; k < n; k += UNROLL_FACTOR) {
                mac(A[IDX(i, k, n)], B[IDX(k, j, n)]);
                mac(A[IDX(i, k + 1, n)], B[IDX(k + 1, j, n)]);
                mac(A[IDX(i, k + 2, n)], B[IDX(k + 2, j, n)]);
                mac(A[IDX(i, k + 3, n)], B[IDX(k + 3, j, n)]);
            }
            C[IDX(i, j, n)] = read_acc();
            cx_sel(max_sel);
            C[IDX(i, j, n)] = max(0, C[IDX(i, j, n)]);
            cx_sel(mac_sel);
        }
    }
}

void nn_layer_nn_acc(const int *A, const int *B, int *C, int n, cx_sel_t nn_sel) {
    int temp = 0, k = 0;
    cx_sel(nn_sel);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            reset();
            for (int k = 0; k < n; k += UNROLL_FACTOR) {
                nn_acc(A[IDX(i, k, n)], B[IDX(k, j, n)]);
                nn_acc(A[IDX(i, k + 1, n)], B[IDX(k + 1, j, n)]);
                nn_acc(A[IDX(i, k + 2, n)], B[IDX(k + 2, j, n)]);
                nn_acc(A[IDX(i, k + 3, n)], B[IDX(k + 3, j, n)]);
            }
            C[IDX(i, j, n)] = nn_relu();
        }
    }
}

void run_nn() {
    const int m = SIZE_0;
    const int n = SIZE_1;
    long long count_cx = 0, count_scalar = 0;
    
    int *A = gen_mat(n, m);
    int *B = gen_mat(n, m);
    int *C = alloc_mat(n, m);
    int *B_ref = alloc_mat(n, m);
    int *C_ref = alloc_mat(n, m);

    memcpy(B_ref, B, n * m * sizeof(int));

    struct perf_event_attr pe;
    int fd;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    // pe.exclude_user = 0;
    // pe.exclude_kernel = 1;
    // pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
       fprintf(stderr, "Error opening leader %llx, errno: %d\n", pe.config, errno);
       exit(EXIT_FAILURE);
    }

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    // cx_sel_t nn_acc_sel = cx_open(CX_GUID_NN_ACC, CX_NO_VIRT, -1);
    cx_sel_t mac_sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t max_sel = cx_open(CX_GUID_MAX, CX_NO_VIRT, -1);

    // assert(nn_acc_sel > 0);
    // assert(mac_sel > 0);
    // assert(max_sel > 0);

    for (int i = 0; i < N_LAYERS; i++) {
        nn_layer_mac_relu(A, B, C, n, mac_sel, max_sel);
        // nn_layer_nn_acc(A, B, C, n, nn_acc_sel);
        if (i == N_LAYERS - 1) {
            break;
        }
        memcpy(B, C, sizeof(int) * n * m);
        memset(C, 0, sizeof(int) * n * m);
    }

    // cx_close(nn_acc_sel);
    cx_close(mac_sel);
    cx_close(max_sel);

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count_cx, sizeof(long long));

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    for (int i = 0; i < N_LAYERS; i++) {
        nn_layer_scalar(A, B_ref, C_ref, m, n);
        if (i == N_LAYERS - 1) {
            break;
        }
        memcpy(B_ref, C_ref, sizeof(int) * n * m);
        memset(C_ref, 0, sizeof(int) * n * m);
    }

    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count_scalar, sizeof(long long));

    mat_check_equal(C, C_ref, m, n);

    printf("Insn Counts\nScalar: %lld, CX: %lld\n", count_scalar, count_cx);

    cx_sel(CX_LEGACY);
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    cx_sel_t sel = cx_open(CX_GUID_MAX, CX_NO_VIRT, -1);
    cx_close(sel);
    run_nn();
    printf("NN ran!\n");
    return 0;
}