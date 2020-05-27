//
// Created by inFinity on 2019-11-20.
//

#ifndef KALEIDO_MONITOR_H
#define KALEIDO_MONITOR_H

#include <string>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <sstream>

/**
 * Returns the peak (maximum so far) resident set size (physical
 * memory use) measured in bytes, or zero if the value cannot be
 * determined on this OS.
 */
size_t getPeakRSS() {
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    return (size_t) (rusage.ru_maxrss * 1024L);
}

/**
 * Returns the current resident set size (physical memory use) measured
 * in bytes, or zero if the value cannot be determined on this OS.
 */
size_t getCurrentRSS() {
    /* Linux ---------------------------------------------------- */
    long rss = 0L;
    FILE *fp = nullptr;
    if ((fp = fopen("/proc/self/statm", "r")) == nullptr)
        return (size_t) 0L; /* Can't open? */
    if (fscanf(fp, "%*s%ld", &rss) != 1) {
        fclose(fp);
        return (size_t) 0L; /* Can't read? */
    }
    fclose(fp);
    return (size_t) rss * (size_t) sysconf(_SC_PAGESIZE);
}

static const std::string perf_mem = "imc/event=0x4,umask=0x3/";
inline pid_t perf_pid = 0;

static const std::string perf_all
        = "cycles,instructions,cache-references,cache-misses,bus-cycles,L1-dcache-loads,L1-dcache-load-"
          "misses,L1-dcache-stores,dTLB-loads,dTLB-load-misses,dTLB-prefetch-misses,LLC-loads,LLC-load-"
          "misses,LLC-stores,LLC-prefetches,cycle_activity.stalls_mem_any,uops_retired.stall_cycles,"
          "cycle_activity.stalls_ldm_pending";

class PerfMem {
public:
    PerfMem() {
        std::string filename = "perf.mem";
        std::stringstream s;
        s << getpid();
        perf_pid = fork();
        if (perf_pid == 0) {
            signal(SIGHUP, SIG_IGN);
            auto fd = open("/tmp/log", O_RDWR);
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            exit(execl("/usr/bin/env", "env", "perf", "stat", "-e", perf_mem.c_str(), "-o",
                       filename.c_str(), "-a", "--per-socket", nullptr));
        }
    }

    ~PerfMem() {
        kill(perf_pid, SIGINT);
        waitpid(perf_pid, nullptr, 0);
    }
};

class PerfStat {
public:
    explicit PerfStat(const std::string &events = "") {
        std::string filename = "perf.stat";
        std::stringstream s;
        s << getpid();
        perf_pid = fork();
        if (perf_pid == 0) {
            signal(SIGHUP, SIG_IGN);
            auto fd = open("/dev/null", O_RDWR);
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (!events.empty())
                exit(execl("/usr/bin/env", "env", "perf", "stat", "-e", events.c_str(), "-o",
                           filename.c_str(), "-p", s.str().c_str(), nullptr));
            else
                exit(execl("/usr/bin/env", "env", "perf", "stat", "-o", filename.c_str(), "-p",
                           s.str().c_str(), nullptr));
        }
    }

    ~PerfStat() {
        kill(perf_pid, SIGINT);
        waitpid(perf_pid, nullptr, 0);
    }
};

class PerfRecord {
public:
    explicit PerfRecord(std::string events = "cache-misses") {
        std::string filename = "perf.data";
        std::stringstream s;
        s << getpid();
        perf_pid = fork();
        if (perf_pid == 0) {
            signal(SIGHUP, SIG_IGN);
            auto fd = open("/dev/null", O_RDWR);
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (!events.empty())
                exit(execl("/usr/bin/env", "env", "perf", "record", "-e", events.c_str(), "-o",
                           filename.c_str(), "-p", s.str().c_str(), nullptr));
            else
                exit(execl("/usr/bin/env", "env", "perf", "record", "-o", filename.c_str(), "-p",
                           s.str().c_str(), nullptr));
        }
    }

    ~PerfRecord() {
        kill(perf_pid, SIGINT);
        waitpid(perf_pid, nullptr, 0);
    }
};


#endif //KALEIDO_MONITOR_H
