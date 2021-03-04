#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits.h>
#include <numa.h>
#include <numeric>
#include <random>
#include <sched.h>
#include <sys/syscall.h>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <vector>

#include "StopWatch.h"

template <typename... T> struct dependent_false { static constexpr bool value = false; };

template<typename _Tp>
using _Val = std::remove_volatile_t<_Tp>;

template<typename _Tp>
_GLIBCXX_ALWAYS_INLINE bool
compare_exchange_weak(_Tp* __ptr, _Val<_Tp>& __expected,
                      _Val<_Tp> __desired) noexcept
{
    return __atomic_compare_exchange(__ptr, std::__addressof(__expected),
                                     std::__addressof(__desired), true,
                                     int(std::memory_order_seq_cst), int(std::memory_order_relaxed));
}

static void writeAddFloat(Float32* f, _Val<Float32> v){
    alignas(Float32) unsigned char __buf[sizeof(Float32)];
    auto* __dest = reinterpret_cast<_Val<Float32>*>(__buf);
    __atomic_load(f, __dest, int(std::memory_order_relaxed));
    _Val<Float32> __oldval = *__dest;
    _Val<Float32> __newval = __oldval + v;
    while (!compare_exchange_weak(f, __oldval, __newval))
        __newval = __oldval + v;
}

static void writeAddFloat(Float64* f, _Val<Float64> v){
    alignas(Float64) unsigned char __buf[sizeof(Float64)];
    auto* __dest = reinterpret_cast<_Val<Float64>*>(__buf);
    __atomic_load(f, __dest, int(std::memory_order_relaxed));
    _Val<Float64> __oldval = *__dest;
    _Val<Float64> __newval = __oldval + v;
    while (!compare_exchange_weak(f, __oldval, __newval))
        __newval = __oldval + v;
}

// NOTE: This function does not fit for Floating
using ab8 __attribute__((__may_alias__)) = uint8_t;
using ab32 __attribute__((__may_alias__)) = uint32_t;
using ab64 __attribute__((__may_alias__)) = uint64_t;
template <typename T> static bool CAS(T* t, T oldv, T newv) {
    if constexpr (sizeof(T) == 1) {
        return __sync_bool_compare_and_swap(reinterpret_cast<ab8*>(t),
            *reinterpret_cast<ab8*>(&oldv), *reinterpret_cast<ab8*>(&newv));
    } else if constexpr (sizeof(T) == 4) {
        return __sync_bool_compare_and_swap(reinterpret_cast<ab32*>(t),
            *reinterpret_cast<ab32*>(&oldv), *reinterpret_cast<ab32*>(&newv));
    } else if constexpr (sizeof(T) == 8) {
        return __sync_bool_compare_and_swap(reinterpret_cast<ab64*>(t),
            *reinterpret_cast<ab64*>(&oldv), *reinterpret_cast<ab64*>(&newv));
    } else {
        static_assert(dependent_false<T>::value, "CAS Pointer should have length 1, 4 or 8.");
    }
}

template <typename T, typename F> static bool writeCond(T* t, T newv, F&& f) {
    T oldv;
    bool r = 0;
    do {
        oldv = *t;
    } while (std::forward<F>(f)(oldv, newv) && !(r = CAS(t, oldv, newv)));
    return r;
}

template <typename T> static bool write(T* a, T b) {
    return writeCond(a, b, [&](T, T) { return true; });
}

template <typename T> static bool writeMin(T* a, T b) { return writeCond(a, b, std::greater<T>()); }

template <typename T> static bool writeMax(T* a, T b) { return writeCond(a, b, std::less<T>()); }

template <typename T> static void writeAdd(T* a, T b) {
    T newv, oldv;
    do {
        oldv = *a;
        newv = oldv + b;
    } while (!CAS(a, oldv, newv));
}

template<>
void writeAdd(Float32* a, Float32 b) {
    writeAddFloat(a, b);
}

template<>
void writeAdd(Float64* a, Float64 b) {
    writeAddFloat(a, b);
}

template <typename T> static T fetchAdd(T* a, T b) {
    T newv, oldv;
    do {
        oldv = *a;
        newv = oldv + b;
    } while (!CAS(a, oldv, newv));
    return oldv;
}

inline int get_thread_id() { return syscall(SYS_gettid); }

inline uint64_t rdtsc() {
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_PRIVATE_FLAG 128
static int ab_futex_wake = FUTEX_WAKE | FUTEX_PRIVATE_FLAG;
static int ab_futex_wait = FUTEX_WAIT | FUTEX_PRIVATE_FLAG;
static long long ab_spin_count_var = 300000LL; // 3msec
// static long long ab_spin_count_var = 300LL; // 3msec

static constexpr int SZ = 1;
static constexpr size_t CACHE_LINE_SIZE = 128; // xeon cache prefetch 1

inline size_t ab_get_num_procs(void) { return sysconf(_SC_NPROCESSORS_ONLN) / SZ; }

enum memmodel {
    MEMMODEL_RELAXED = 0,
    MEMMODEL_CONSUME = 1,
    MEMMODEL_ACQUIRE = 2,
    MEMMODEL_RELEASE = 3,
    MEMMODEL_ACQ_REL = 4,
    MEMMODEL_SEQ_CST = 5
};

inline void futex_wait(int* addr, int val) {
    int err = syscall(SYS_futex, addr, ab_futex_wait, val, NULL);
    if (__builtin_expect(err < 0 && errno == ENOSYS, 0)) {
        ab_futex_wait &= ~FUTEX_PRIVATE_FLAG;
        ab_futex_wake &= ~FUTEX_PRIVATE_FLAG;
        syscall(SYS_futex, addr, ab_futex_wait, val, NULL);
    }
}

inline void futex_wake(int* addr, int count) {
    int err = syscall(SYS_futex, addr, ab_futex_wake, count);
    if (__builtin_expect(err < 0 && errno == ENOSYS, 0)) {
        ab_futex_wait &= ~FUTEX_PRIVATE_FLAG;
        ab_futex_wake &= ~FUTEX_PRIVATE_FLAG;
        syscall(SYS_futex, addr, ab_futex_wake, count);
    }
}

inline void cpu_relax(void) { __asm volatile("" : : : "memory"); }

inline int do_spin(int* addr, int val) {
    unsigned long long i, count = ab_spin_count_var;
    for (i = 0; i < count; i++)
        if (__builtin_expect(__atomic_load_n(addr, MEMMODEL_RELAXED) != val, 0))
            return 0;
        else
            cpu_relax();
    return 1;
}

inline void do_wait(int* addr, int val) {
    if (do_spin(addr, val))
        futex_wait(addr, val);
}

typedef struct {
    /* Make sure total/generation is in a mostly read cacheline, while
       awaited in a separate cacheline.  */
    alignas(CACHE_LINE_SIZE) unsigned total;
    alignas(CACHE_LINE_SIZE) unsigned generation;
    alignas(CACHE_LINE_SIZE) unsigned awaited;
    alignas(CACHE_LINE_SIZE) bool spin;
} ab_barrier_t;

typedef unsigned int ab_barrier_state_t;

#define BAR_WAS_LAST 256
#define BAR_INCR 512
#define BAR_OVERFLOW 10000000u

inline void ab_barrier_init(ab_barrier_t* bar, unsigned count, bool spin) {
    bar->total = count;
    bar->awaited = count;
    bar->generation = 0;
    bar->spin = spin;
}

inline void ab_barrier_reinit(ab_barrier_t* bar, unsigned count) {
    __atomic_add_fetch(&bar->awaited, count - bar->total, MEMMODEL_ACQ_REL);
    bar->total = count;
}

inline ab_barrier_state_t ab_barrier_wait_start(ab_barrier_t* bar) {
    unsigned int ret = __atomic_load_n(&bar->generation, MEMMODEL_ACQUIRE);
    ret &= -BAR_INCR;
    if (__atomic_add_fetch(&bar->awaited, -1, MEMMODEL_ACQUIRE) == 0)
        ret |= BAR_WAS_LAST;
    return ret;
}

inline uint64_t ab_barrier_wait_end(ab_barrier_t* bar, ab_barrier_state_t state) {
    if (__builtin_expect(state & BAR_WAS_LAST, 0)) {
        auto ret = rdtsc();
        bar->awaited = bar->total;
        __atomic_store_n(&bar->generation, bar->generation + BAR_INCR, MEMMODEL_RELEASE);
        if (bar->total > 1) // fast path
        {
            futex_wake((int*)&bar->generation, INT_MAX);
        }
        return rdtsc() - ret;
    } else {
        if (bar->awaited > BAR_OVERFLOW)
            return 0;
        auto ret = rdtsc();
        if (bar->spin)
            do
                do_wait((int*)&bar->generation, state);
            while (__atomic_load_n(&bar->generation, MEMMODEL_ACQUIRE) == state);
        else
            do
                futex_wait((int*)&bar->generation, state);
            while (__atomic_load_n(&bar->generation, MEMMODEL_ACQUIRE) == state);
        return rdtsc() - ret;
    }
}

inline uint64_t ab_barrier_wait(ab_barrier_t* bar) {
    return ab_barrier_wait_end(bar, ab_barrier_wait_start(bar));
}

inline uint64_t ab_barrier_wait_last(ab_barrier_t* bar) {
    ab_barrier_state_t state = ab_barrier_wait_start(bar);
    if (state & BAR_WAS_LAST)
        return ab_barrier_wait_end(bar, state);
    return 0;
}

class Tasks {
public:
    virtual void run(size_t id) = 0;
    virtual ~Tasks() {}
};

struct Range {
    size_t b;
    size_t e;
    char x[CACHE_LINE_SIZE - 2 * sizeof(size_t)];
    Range() {}
    Range(size_t b_, size_t e_)
        : b(b_)
        , e(e_) {}
};

template <class F> class TasksImpl : public Tasks {
public:
    TasksImpl(const std::vector<Range>& ranges_, size_t chunk_size_, bool worksteal_, F&& f_)
        : ranges(ranges_)
        , chunk_size(chunk_size_)
        , worksteal(worksteal_)
        , f(std::forward<F>(f_)) {}
    TasksImpl(std::vector<Range>&& ranges_, size_t chunk_size_, bool worksteal_, F&& f_)
        : ranges(std::move(ranges_))
        , chunk_size(chunk_size_)
        , worksteal(worksteal_)
        , f(std::forward<F>(f_)) {}
    void run(size_t id) {
        if (worksteal) {
            auto work = [&](size_t t_i) {
                while (true) {
                    size_t b_i = __sync_fetch_and_add(&ranges[t_i].b, chunk_size);
                    if (b_i >= ranges[t_i].e)
                        break;
                    size_t begin_b_i = b_i;
                    size_t end_b_i = b_i + chunk_size;
                    if (end_b_i > ranges[t_i].e) {
                        end_b_i = ranges[t_i].e;
                    }
                    for (b_i = begin_b_i; b_i < end_b_i; b_i++)
                        f(b_i);
                }
            };
            work(id);
            for (size_t t_offset = 1; t_offset < ranges.size(); t_offset++) {
                size_t t_i = (id + t_offset) % ranges.size();
                work(t_i);
            }
        } else {
            for (auto i = ranges[id].b; i < ranges[id].e; ++i)
                f(i);
        }
    }

private:
    std::vector<Range> ranges;
    size_t chunk_size;
    bool worksteal;
    F f;
};

class Gang {
    friend class GangUtil;

public:
    inline thread_local static unsigned int thread_id;
    inline thread_local static unsigned int cpu_id;

    Gang(size_t num_, size_t cpu_num_, bool bind_ = true)
        : num(num_)
        , cpu_num(cpu_num_)
        , bind(bind_) {
        thread_id = 0;
        cpu_id = 0;
        CPU_ZERO(&all);
        for (auto i = 0u; i < num; ++i) {
            CPU_SET(i * SZ, &all);
        }
        ab_barrier_init(&abb, num, true);
        ab_barrier_init(&abb2, num, true);
        ab_barrier_init(&middle, num, true);
        for (auto i = 1u; i < num; ++i) {
            ts.emplace_back(&Gang::loop, this, i);
        }
    }

    Gang(int socket_id)
        : num(numa_num_configured_cpus() / numa_num_configured_nodes())
        , bind(true)
        , socket_id(socket_id) {
        thread_id = 0;
        cpu_id = 0;
        CPU_ZERO(&all);
        for (auto i = 0u; i < num; ++i) {
            CPU_SET(i * SZ, &all);
        }
        ab_barrier_init(&abb, num, true);
        ab_barrier_init(&abb2, num, true);
        ab_barrier_init(&middle, num, true);
        for (auto i = 1u; i < num; ++i) {
            ts.emplace_back(&Gang::loop, this, i);
        }
    }

    template <class F> void submit(size_t end, F&& f, size_t granular, bool worksteal) {
        auto n = std::min((end + granular - 1) / granular, num);
        if (n == 0)
            return;
        auto pn = end / n;
        std::vector<size_t> sz(n, pn);
        for (auto i = 0u; i < end % n; ++i) {
            sz[i]++;
        }
        size_t b = 0;
        std::vector<Range> ranges;
        ranges.reserve(n);
        for (auto i = 0u; i < n; ++i) {
            ranges.emplace_back(b, b + sz[i]);
            b += sz[i];
        }
        tasks.reset(new TasksImpl<F>(std::move(ranges), granular, worksteal, std::forward<F>(f)));
        if (n == 1) {
            tasks->run(0);
        } else {
            reinit(n);
            run_main();
        }
    }

    template <class F>
    void submit(size_t start, size_t end, F&& f, size_t granular, bool worksteal) {
        auto n = std::min((end - start + granular - 1) / granular, num);
        if (n == 0)
            return;
        auto pn = (end - start) / n;
        std::vector<size_t> sz(n, pn);
        for (auto i = 0u; i < (end - start) % n; ++i) {
            sz[i]++;
        }
        size_t b = start;
        std::vector<Range> ranges;
        ranges.reserve(n);
        for (auto i = 0u; i < n; ++i) {
            ranges.emplace_back(b, b + sz[i]);
            b += sz[i];
        }
        tasks.reset(new TasksImpl<F>(std::move(ranges), granular, worksteal, std::forward<F>(f)));
        if (n == 1) {
            tasks->run(0);
        } else {
            reinit(n);
            run_main();
        }
    }

    template <class F> void submitPerCpu(size_t end, F&& f, size_t granular, bool worksteal) {
        auto n = std::min((end + granular - 1) / granular, num / cpu_num);
        if (n == 0)
            return;
        if (n % cpu_num) {
            std::cerr << "Cannot use submitPerCpu when gang work num isn't a multiplier of cpu num"
                      << std::endl;
            abort();
        }

        auto pn = end / n;
        std::vector<size_t> sz(n, pn);
        for (auto i = 0u; i < end % n; ++i) {
            sz[i]++;
        }
        size_t b = 0;
        std::vector<Range> ranges;
        ranges.reserve(n);
        for (auto i = 0u; i < n; ++i) {
            ranges.emplace_back(b, b + sz[i]);
            b += sz[i];
        }
        for (auto x = 0u; x < cpu_num - 1; ++x)
            for (auto i = 0u; i < n; ++i)
                ranges.emplace_back(ranges[i]);
        tasks.reset(new TasksImpl<F>(std::move(ranges), granular, worksteal, std::forward<F>(f)));
        if (n == 1) {
            tasks->run(0);
        } else {
            reinit(n);
            run_main();
        }
    }

    ~Gang() {
        shutdown = true;
        // exit threads that are waiting or gonna wait on the barrier
        // this is complicated when using a second variable barrier for task synchronization.
        // awaited counter is used to make sure all threads are exited

        abb.total = 1;
        // after this point, all subsequent ab_barrier_wait call will return immediately.
        // Thus we can be sure there is no thread that is currently calling ab_barrier_wait_end,
        // since thread num < num.
        for (auto i = 0u; i < num; ++i) {
            // start decrementing awaited counter in order to make sure there is one thread calling
            // the futex wake.
            ab_barrier_state_t state = ab_barrier_wait_start(&abb);
            if (state & BAR_WAS_LAST) {
                abb.awaited = abb.total;
                __atomic_store_n(&abb.generation, abb.generation + BAR_INCR, MEMMODEL_RELEASE);
                futex_wake((int*)&abb.generation, INT_MAX);
                break;
            } else if (abb.awaited > BAR_OVERFLOW)
                break;
        }
        for (auto& t : ts) {
            t.join();
        }
    }

private:
    ab_barrier_t abb;
    ab_barrier_t abb2;
    ab_barrier_t middle;
    alignas(CACHE_LINE_SIZE) unsigned flag;
    size_t num;
    size_t cpu_num;
    std::unique_ptr<Tasks> tasks;
    bool shutdown = false;
    std::vector<std::thread> ts;
    bool bind;
    cpu_set_t all;
    int socket_id{ -1 };

    void loop(size_t id) {
        thread_id = id * SZ;
        cpu_id = thread_id & (cpu_num - 1); // hardcode
        if (bind) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            if (socket_id == -1)
                CPU_SET(thread_id, &cpuset);
            else
                CPU_SET(thread_id * numa_num_configured_nodes() + socket_id, &cpuset);
            int rc = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
            if (rc != 0) {
                std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
            }
        }
        try {
            while (true) {
                run(id);
            }
        } catch (...) {
        }
    }

    void run_main() {
        if (bind) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            if (socket_id == -1)
                CPU_SET(0, &cpuset);
            else
                CPU_SET(socket_id, &cpuset);
            sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
        }
        run(0);

        if (bind) {
            if (socket_id == -1)
                sched_setaffinity(0, sizeof(cpu_set_t), &all);
        }
    }

    void run(size_t id) {
        ab_barrier_wait(&abb);
        if (shutdown)
            throw "quit";
        auto state = __atomic_load_n(&abb.generation, MEMMODEL_ACQUIRE);
        auto ret = __atomic_load_n(&flag, MEMMODEL_ACQUIRE);
        if ((ret & -BAR_INCR) > (state & -BAR_INCR) || id >= (ret & (BAR_WAS_LAST - 1)))
            return;
        tasks->run(id);
        ab_barrier_wait(&abb2);
    }

    void reinit(size_t num) {
        auto state = abb.generation + BAR_INCR;
        state &= -BAR_INCR;
        state |= num;
        __atomic_store_n(&flag, state, MEMMODEL_RELEASE);

        abb2.total = num;
        __atomic_store_n(&abb2.awaited, num, MEMMODEL_RELEASE);
    }
};

static void ab_alloc(void** memptr, size_t alignment, size_t size) {
    int res = posix_memalign(memptr, alignment, size);
    if (0 != res) {
        std::cerr << "Cannot allocate memory (posix_memalign)" << std::endl;
        abort();
    }
}

inline std::unique_ptr<Gang> create_gang(size_t thread_num, size_t cpu_num, bool bind = false) {
    void* buf = nullptr;
    ab_alloc(&buf, CACHE_LINE_SIZE, sizeof(Gang));
    std::unique_ptr<Gang> ret;
    ret.reset(new (buf) Gang(thread_num, cpu_num, bind));
    return ret;
}

inline std::unique_ptr<Gang> create_gang(int socket_id) {
    void* buf = nullptr;
    ab_alloc(&buf, CACHE_LINE_SIZE, sizeof(Gang));
    std::unique_ptr<Gang> ret;
    ret.reset(new (buf) Gang(socket_id));
    return ret;
}

struct GangUtil {
    std::unique_ptr<Gang> g{};
    int socket_id;
    bool worksteal{ true };
    size_t SMALL_SIZE = 2048;
    int thread_num;

    static constexpr size_t BLOCK_SIZE = 2048;

    GangUtil(int socket_id = -1)
        : socket_id(socket_id) {
#ifdef NUMA
        if (socket_id == -1) {
#else
        if (true) {
#endif
            g = create_gang(numa_num_configured_cpus(), numa_num_configured_nodes());
            thread_num = numa_num_configured_cpus();
        } else {
            g = create_gang(socket_id);
            thread_num = numa_num_configured_cpus() / numa_num_configured_nodes();
        }
    }

    template <class F> void submit(size_t start, size_t end, F&& f, size_t granular) {
        g->submit(start, end, std::forward<F>(f), granular, worksteal);
    }

    template <class F> void submit(size_t start, size_t end, F&& f) {
        g->submit(start, end, std::forward<F>(f), SMALL_SIZE, worksteal);
    }

    template <class F> void submit(size_t end, F&& f, size_t granular) {
        g->submit(end, std::forward<F>(f), granular, worksteal);
    }

    template <class F> void submit(size_t end, F&& f) {
        g->submit(end, std::forward<F>(f), SMALL_SIZE, worksteal);
    }

    template <class F> void submitPerCpu(size_t end, F&& f) {
        g->submitPerCpu(end, std::forward<F>(f), SMALL_SIZE, worksteal);
    }

    template <class F> void groupSubmit(size_t end, F&& f) {
        g->submit(end, std::forward<F>(f), 1, worksteal);
    }

    template <typename T> void touch(T* data, size_t size) {
        static constexpr size_t PAGESIZE = 1 << 21; // huge page
        auto pn = size * sizeof(T) / PAGESIZE;
        submit(pn, [&](size_t i) { reinterpret_cast<char*>(data)[i * PAGESIZE] = 0; });
    }

    template <typename T> struct plusf {
        constexpr T operator()(const T& a, const T& b) const { return a + b; }
    };
    template <typename T> struct minf {
        constexpr T operator()(const T& a, const T& b) const { return (a < b) ? a : b; }
    };
    template <typename T> struct maxf {
        constexpr T operator()(const T& a, const T& b) const { return (a > b) ? a : b; }
    };
    template <typename T> struct identf {
        T operator()(const T& x) { return x; }
    };

    template <typename T, typename Z, typename F> T reduceSerial(T* arr, size_t n, Z zero, F&& f) {
        T r = zero;
        for (size_t i = 0; i < n; ++i)
            r = f(r, arr[i]);
        return r;
    }

    template <typename T, typename Z, typename F> T reduce(T* arr, size_t n, Z zero, F&& f) {
        size_t nb = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
        if (nb <= 1)
            return reduceSerial(arr, n, zero, std::forward<F>(f));
        T* sums = nullptr;
        ab_alloc((void**)&sums, CACHE_LINE_SIZE, nb * sizeof(T));
        groupSubmit(nb, [&](size_t j) {
            auto i = j * BLOCK_SIZE;
            auto len = (j == nb - 1) ? n - i : BLOCK_SIZE;
            sums[j] = reduceSerial(arr + i, len, zero, std::forward<F>(f));
        });
        auto r = reduce(sums, nb, zero, std::forward<F>(f));
        free(sums);
        return r;
    }

    template <typename T> T reduce(T* arr, size_t n) {
        return reduce(arr, n, (T)0, std::plus<T>());
    }

    template <typename T, typename U = T> void assign(T* arr, size_t n, U u = 0) {
        T v = u;
        submit(n, [&](size_t i) { arr[i] = v; });
    }

    template <typename T, typename Z, typename U, typename F>
    T scanSerial(T* out, U* in, size_t n, Z zero, F&& f, bool inclusive, bool back) {
        static_assert(std::is_integral_v<T> && std::is_integral_v<U>, "Integral required.");
        static_assert(sizeof(T) >= sizeof(U), "output type is smaller than input.");

        auto r = zero;
        if (inclusive) {
            if (back) {
                for (size_t i = n; i-- > 0;)
                    out[i] = r = std::forward<F>(f)(r, in[i]);
            } else
                for (size_t i = 0; i < n; ++i)
                    out[i] = r = std::forward<F>(f)(r, in[i]);
        } else {
            if (back) {
                for (size_t i = n; i-- > 0;) {
                    auto x = in[i]; // out and in may point to the same place
                    out[i] = r;
                    r = std::forward<F>(f)(r, x);
                }
            } else
                for (size_t i = 0; i < n; ++i) {
                    auto x = in[i]; // out and in may point to the same place
                    out[i] = r;
                    r = std::forward<F>(f)(r, x);
                }
        }
        return r;
    }

    template <typename T, typename Z, typename U, typename F>
    T scan(T* out, U* in, size_t n, Z zero, F&& f, bool inclusive, bool back) {
        auto nb = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
        if (nb <= 1)
            return scanSerial(out, in, n, zero, std::forward<F>(f), inclusive, back);
        T* sums = nullptr;
        ab_alloc((void**)&sums, CACHE_LINE_SIZE, nb * sizeof(T));
        groupSubmit(nb, [&](size_t j) {
            auto i = j * BLOCK_SIZE;
            auto len = (j == nb - 1) ? n - i : BLOCK_SIZE;
            auto s = &in[i];
            sums[j] = reduceSerial(s, len, (U)0, std::forward<F>(f));
        });
        auto total = scan(sums, sums, nb, zero, std::forward<F>(f), false, back);
        groupSubmit(nb, [&](size_t j) {
            auto i = j * BLOCK_SIZE;
            auto len = (j == nb - 1) ? n - i : BLOCK_SIZE;
            scanSerial(&out[i], &in[i], len, sums[j], std::forward<F>(f), inclusive, back);
        });
        free(sums);
        return total;
    }

    template <typename T, typename Z, typename U, typename F>
    T scan(T* out, U* in, size_t n, Z zero, F&& f) {
        return scan(out, in, n, zero, std::forward<F>(f), false, false);
    }
    template <typename T, typename Z, typename U, typename F>
    T scanI(T* out, U* in, size_t n, Z zero, F&& f) {
        return scan(out, in, n, zero, std::forward<F>(f), true, false);
    }
    template <typename T, typename Z, typename U, typename F>
    T scanBack(T* out, U* in, size_t n, Z zero, F&& f) {
        return scan(out, in, n, zero, std::forward<F>(f), false, true);
    }
    template <typename T, typename Z, typename U, typename F>
    T scanIBack(T* out, U* in, size_t n, Z zero, F&& f) {
        return scan(out, in, n, zero, std::forward<F>(f), true, true);
    }
    template <typename T, typename Z, typename U> T scan(T* out, U* in, size_t n, Z zero) {
        return scan(out, in, n, zero, std::plus<T>(), false, false);
    }
    template <typename T, typename Z, typename U> T scanI(T* out, U* in, size_t n, Z zero) {
        return scan(out, in, n, zero, std::plus<T>(), true, false);
    }
    template <typename T, typename Z, typename U> T scanBack(T* out, U* in, size_t n, Z zero) {
        return scan(out, in, n, zero, std::plus<T>(), false, true);
    }
    template <typename T, typename Z, typename U> T scanIBack(T* out, U* in, size_t n, Z zero) {
        return scan(out, in, n, zero, std::plus<T>(), true, true);
    }

    template <typename T> void iota(T* in, size_t n) {
        submit(n, [&](auto i) { in[i] = i; });
    }

    template <typename T> size_t countSerial(T* in, size_t n) {
        return reduceSerial(in, n, (T)0,
            [&](T a, T b) { return a + (b == std::numeric_limits<T>::max() ? 0 : 1); });
    }

    size_t countSerial(bool* in, size_t n) {
        size_t r = 0;
        if ((n & 1023) == 0 && (reinterpret_cast<size_t>(&in[0]) & 7) == 0) {
            auto* ppi = reinterpret_cast<std::uint64_t*>(&in[0]);
            for (size_t k = 0; k < n / 1024; k++) {
                auto pi = ppi + k * 128;
                std::uint64_t rr = 0;
                for (size_t j = 0; j < 128; j++)
                    rr += pi[j];
                r += (rr & 255) + ((rr >> 8) & 255) + ((rr >> 16) & 255) + ((rr >> 24) & 255)
                    + ((rr >> 32) & 255) + ((rr >> 40) & 255) + ((rr >> 48) & 255)
                    + ((rr >> 56) & 255);
            }
        } else if ((n & 511) == 0 && (reinterpret_cast<size_t>(&in[0]) & 3) == 0) {
            auto* ppi = reinterpret_cast<std::uint32_t*>(&in[0]);
            for (size_t k = 0; k < n / 512; k++) {
                auto pi = ppi + k * 128;
                std::uint32_t rr = 0;
                for (size_t j = 0; j < 128; j++)
                    rr += pi[j];
                r += (rr & 255) + ((rr >> 8) & 255) + ((rr >> 16) & 255) + ((rr >> 24) & 255);
            }
        } else if ((n & 255) == 0 && (reinterpret_cast<size_t>(&in[0]) & 1) == 0) {
            auto* ppi = reinterpret_cast<std::uint16_t*>(&in[0]);
            for (size_t k = 0; k < n / 256; k++) {
                auto pi = ppi + k * 128;
                std::uint16_t rr = 0;
                for (size_t j = 0; j < 128; j++)
                    rr += pi[j];
                r += (rr & 255) + ((rr >> 8) & 255);
            }
        } else
            for (size_t j = 0; j < n; j++)
                r += in[j];
        return r;
    }

    template <typename T> size_t count(T* in, size_t n) {
        auto nb = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
        if (nb <= 1) {
            return countSerial(in, n);
        }
        size_t* sums = nullptr;
        ab_alloc((void**)&sums, CACHE_LINE_SIZE, nb * sizeof(size_t));
        groupSubmit(nb, [&](size_t j) {
            auto i = j * BLOCK_SIZE;
            auto len = (j == nb - 1) ? n - i : BLOCK_SIZE;
            sums[j] = countSerial(&in[i], len);
        });
        size_t r = reduceSerial(sums, nb, (size_t)0, std::plus<size_t>());
        free(sums);
        return r;
    }

    template <typename T, typename U, typename F>
    size_t packSerial(T* out, U* in, size_t b, size_t e, F&& f) {
        size_t k = 0;
        for (size_t i = b; i < e; i++)
            if (in[i] != std::numeric_limits<U>::max())
                out[k++] = std::forward<F>(f)(i);
        return k;
    }

    template <typename T, typename F>
    size_t packSerial(T* out, bool* in, size_t b, size_t e, F&& f) {
        size_t k = 0;
        for (size_t i = b; i < e; i++)
            if (in[i])
                out[k++] = std::forward<F>(f)(i);
        return k;
    }

    template <typename T, typename U, typename F> size_t pack(T*&& out, U* in, size_t n, F&& f) {
        if (std::is_rvalue_reference_v<decltype(out)> && out == nullptr)
            throw "invalid";
        auto nb = (n + BLOCK_SIZE - 1) / BLOCK_SIZE;
        if (nb <= 1) {
            if (out == nullptr) {
                auto len = countSerial(in, n);
                ab_alloc((void**)&out, CACHE_LINE_SIZE, len * sizeof(T));
            }
            return packSerial(out, in, 0, n, std::forward<F>(f));
        }
        size_t* sums = nullptr;
        ab_alloc((void**)&sums, CACHE_LINE_SIZE, nb * sizeof(size_t));
        groupSubmit(nb, [&](size_t j) {
            auto i = j * BLOCK_SIZE;
            auto len = (j == nb - 1) ? n - i : BLOCK_SIZE;
            sums[j] = countSerial(&in[i], len);
        });
        size_t r = scan(sums, sums, nb, (size_t)0);
        if (out == nullptr)
            ab_alloc((void**)&out, CACHE_LINE_SIZE, r * sizeof(T));
        groupSubmit(nb, [&](size_t j) {
            auto i = j * BLOCK_SIZE;
            auto len = (j == nb - 1) ? n - i : BLOCK_SIZE;
            packSerial(
                out + sums[j], in, i, i + len, std::forward<F>(f)); // in base addr, start, end
        });
        free(sums);
        return r;
    }

    template <typename T, typename U> size_t packIndex(T*&& out, U* in, size_t n) {
        auto ret = pack(std::forward<T*>(out), in, n, identf<T>());
        return ret;
    }

    template <typename T> size_t filter(T*&& out, T* in, size_t n) {
        auto ret = pack(std::forward<T*>(out), in, n, [&](size_t i) { return in[i]; });
        return ret;
    }

    // Remove duplicate integers in [0,...,n-1].
    // Assumes that flags is already allocated and cleared to SENTINEL.
    // Sets all duplicate values in the array to SENTINEL and resets flags to
    // SENTINEL.
    template <typename T> void dedup(T* data, size_t* index, size_t n) {
        submit(n, [&](size_t i) {
            T key = data[i];
            if (key != std::numeric_limits<T>::max()
                && index[key] == std::numeric_limits<size_t>::max()) {
                CAS(&index[key], std::numeric_limits<size_t>::max(), i);
            }
        });
        // reset flags
        submit(n, [&](size_t i) {
            T key = data[i];
            if (key != std::numeric_limits<T>::max()) {
                if (index[key] == i) { // win
                    index[key] = std::numeric_limits<size_t>::max(); // reset
                } else {
                    data[i] = std::numeric_limits<T>::max(); // lost
                }
            }
        });
    }
};
