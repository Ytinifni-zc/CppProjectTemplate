//
// Created by inFinity on 2019/12/11.
//

#ifndef KALEIDO_NUMA_H
#define KALEIDO_NUMA_H

#include <sched.h>
#include <pthread.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif

void bindProcessCPU(pid_t pid, size_t mask = 0) {
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(mask, &cpu_mask);
    if (sched_setaffinity(pid, sizeof(cpu_mask), &cpu_mask) < 0)
        perror("sched_setaffinity");
}

void bindCurrentProcessCPU(size_t mask = 0) {
    bindProcessCPU(0, mask);
}

void bindThreadCPU(pthread_t thread, size_t mask = 0) {
    cpu_set_t cpu_mask;
    CPU_ZERO(&cpu_mask);
    CPU_SET(mask, &cpu_mask);
    if (pthread_setaffinity_np(thread, sizeof(cpu_mask), &cpu_mask) < 0)
        perror("pthread_setaffinity_np");
}

void bindCurrentThreadCPU(size_t mask = 0) {
    bindThreadCPU(pthread_self(), mask);
}

#endif //KALEIDO_NUMA_H
