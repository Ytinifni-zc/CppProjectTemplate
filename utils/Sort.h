//
// Created by inFinity on 2020/5/26.
//

#pragma once

#include <parallel/algorithm>
#include <ips4o/ips4o.hpp>
#include <algorithm>

#define IPSO_SORT
//#define GNU_SORT

namespace cpp_template {
    template<class RandomIt>
    void sort(RandomIt first, RandomIt last) {
#ifdef GNU_SORT
        __gnu_parallel::sort(first, last);
#elif defined(IPSO_SORT)
        ips4o::parallel::sort(first, last);
#else
        std::sort(first, last);
#endif
    }

    template<class RandomIt, class Compare>
    void sort(RandomIt first, RandomIt last, Compare comp) {
#ifdef GNU_SORT
        __gnu_parallel::sort(first, last, comp);
#elif defined(IPSO_SORT)
        ips4o::parallel::sort(first, last, comp);
#else
        std::sort(first, last, comp);
#endif
    }

}

