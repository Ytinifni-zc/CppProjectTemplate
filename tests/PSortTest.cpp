//
// Created by inFinity on 2020/5/25.
//

#include <parallel/algorithm>
#include <ips4o/ips4o.hpp>
#include <vector>
#include <utils/MapFile.h>
#include <utils/StopWatch.h>
#include <utils/Gang.h>
#include <utils/Monitor.h>
#include <utils/Log.h>

#include "gtest/gtest.h"


TEST(PSort, GNU_Sort) {
    auto mInitialPeakRss = getPeakRSS();
    Stopwatch w;
    size_t length;
    using DataType = int;
    auto addr = mmap_file<DataType *>("random.txt", length);
    std::vector<int> data(length/sizeof(DataType));
    GangUtil gu;
    gu.submit(data.size(), [&](UInt32 i) {data[i] = addr[i];});
    munmap_file(addr, length);
    LOG::info("Copy using: {:.2f}ms", w.elapsedSeconds());
    w.restart();

    __gnu_parallel::sort(data.begin(), data.end());
    LOG::info("GNU Parallel sort using: {:.2f}ms", w.elapsedSeconds());
    auto peakRss = getPeakRSS();
    if (peakRss > mInitialPeakRss) {
        peakRss -= mInitialPeakRss;
    } else {
        peakRss = 0;
    }
    LOG::info("Peak Memory = {:.1f}MB", peakRss / 1048576.0);
}

TEST(PSort, ips4o_Sort) {
    auto mInitialPeakRss = getPeakRSS();
    Stopwatch w;
    size_t length;
    using DataType = int;
    auto addr = mmap_file<DataType *>("random.txt", length);
    std::vector<int> data(length/sizeof(DataType));
    GangUtil gu;
    gu.submit(data.size(), [&](UInt32 i) {data[i] = addr[i];});
    munmap_file(addr, length);
    LOG::info("Copy using: {:.2f}ms", w.elapsedSeconds());
    w.restart();

    ips4o::parallel::sort(data.begin(), data.end());
    LOG::info("IPS4o Parallel sort using: {:.2f}ms", w.elapsedSeconds());
    auto peakRss = getPeakRSS();
    if (peakRss > mInitialPeakRss) {
        peakRss -= mInitialPeakRss;
    } else {
        peakRss = 0;
    }
    LOG::info("Peak Memory = {:.1f}MB", peakRss / 1048576.0);
}
