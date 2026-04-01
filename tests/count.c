#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"
#include "../zoo/muldiv/muldiv.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

#define N 10000LL
static int res = 0, a = 3, b = 5;

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
}

static int init_perf_counter(int kern) {
    int fd;
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe.disabled = 1;
    // pe.exclude_user = 0;
    if (!kern) {
        pe.exclude_kernel = 1;
    }
    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
       fprintf(stderr, "Error opening leader %llx, errno: %d\n", pe.config, errno);
       exit(EXIT_FAILURE);
    }

    return fd;
}

static long long get_overhead_insn_count() {
    
    int fd = init_perf_counter(0);
    
    long long count = 0, count_overhead = 0;
    for (long long i = 0; i < N; i++) {
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(long long));
        count_overhead += count;
    }
    close(fd);
    return count_overhead;
}

static long long get_first_use_insn_count() {
    
    int fd = init_perf_counter(1);
    cx_sel_t sel = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, -1);
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_INTRA_VIRT, sel);

    assert(sel > 0);
    assert(selA > 0);
    
    long long count = 0, count_first_use = 0;
    for (long long i = 0; i < N; i++) {
        cx_sel(sel);
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        mac(a, a);

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(long long));
        count_first_use += count;
        cx_sel(selA);
        mac(b, b);
    }

    cx_close(sel);
    cx_close(selA);
    close(fd);
    return count_first_use;
}

static long long get_sel_insn_count() {
    
    int fd = init_perf_counter(0);
    cx_sel_t sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, sel);

    assert(sel > 0);
    assert(selA > 0);
    
    long long count = 0, count_first_use = 0;
    for (long long i = 0; i < N; i++) {
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        cx_sel(sel);

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(long long));
        count_first_use += count;
        cx_sel(selA);
    }

    cx_close(sel);
    cx_close(selA);
    close(fd);
    return count_first_use;
}

long long get_open_insn_count() {
    int fd = init_perf_counter(1);
    
    long long count = 0, count_open = 0;
    for (long long i = 0; i < N; i++) {
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        cx_sel_t sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(long long));
        assert(sel > 0);
        count_open += count;
        cx_close(sel);
        cx_sel(CX_LEGACY);
    }
    close(fd);
    return count_open;
}

long long get_close_insn_count() {
    int fd = init_perf_counter(1);
    
    long long count = 0, count_close = 0;
    for (long long i = 0; i < N; i++) {
        cx_sel_t sel = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
        assert(sel > 0);

        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        cx_close(sel);

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(long long));
        count_close += count;
    }
    close(fd);
    return count_close;
}

int
main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }

    // The first select initializes some of the kernel structures, and thus has a higher
    // overhead. We shouldn't count that one.
    cx_sel_t selA = cx_open(CX_GUID_MULACC, CX_NO_VIRT, -1);
    cx_close(selA);

    long long count_overhead = get_overhead_insn_count();

    // long long count_first_use = get_first_use_insn_count();
    long long count_open = get_open_insn_count();
    long long count_sel = get_sel_insn_count();    
    long long count_close = get_close_insn_count();

    printf("Avg. open count: %lld, Avg. sel count: %lld, Avg. close count: %lld\n", (count_open - count_overhead) / N, (count_sel - count_overhead) / N, (count_close - count_overhead) / N);
    // printf("Avg. open time: %.5fus, Avg. close time: %.5fus, Avg. sel time: %.5fus\n", open_ms, close_ms, sel_ms);

}
