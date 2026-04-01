#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sched.h>
#include <string.h>

#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/wait.h>

#include "../include/ci.h"
#include "../zoo/mulacc/mulacc.h"

static int verbose = 0;
#define VLOG(...) do { if (verbose) printf(__VA_ARGS__); } while(0)

#define nloops 1


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
    // if (!kern) {
    //     pe.exclude_kernel = 1;
    // }
    // pe.exclude_hv = 1;

    fd = perf_event_open(&pe, -1, 0, -1, 0);
    if (fd == -1) {
       fprintf(stderr, "Error opening leader %llx, errno: %d\n", pe.config, errno);
       exit(EXIT_FAILURE);
    }

    return fd;
}

int main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] == 'v')
            verbose = 1;
    }
    // measure context switch
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    struct timeval start, end;
    long long count = 0, overhead = 0;

    int first_pipefd[2], second_pipefd[2];
    if (pipe(first_pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    if (pipe(second_pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    cx_sel_t sel = cx_open(CX_GUID_MULACC, CX_INTER_VIRT, -1);
    cx_sel(sel);
    mac(3, 4);
    cx_sel(CX_LEGACY);
    pid_t cpid = fork();
    if (cpid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (cpid == 0) {    // child
        if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &set) == -1) {
            exit(EXIT_FAILURE);
        }
        for (size_t i = 0; i < nloops; i++) {
            read(first_pipefd[0], NULL, 0);
            cx_sel(sel);
            mac(2, 3);
            write(second_pipefd[1], NULL, 0);
        }
    } else {           // parent
        if (sched_setaffinity(getpid(), sizeof(cpu_set_t), &set) == -1) {
            exit(EXIT_FAILURE);
        }

        for (size_t i = 0; i < nloops; i++) {
            write(first_pipefd[1], NULL, 0);
            read(second_pipefd[0], NULL, 0);
        }
        wait(NULL);
        
        int a = sel;
        int fd = init_perf_counter(0);
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
        cx_sel(a);
        mac(a, 0);

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(long long));

        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &overhead, sizeof(long long));
        
        printf("Num isns: %lld\n", count - overhead);
    }
    return 0;
}