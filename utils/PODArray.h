//
// Created by inFinity on 2019/11/26.
//

#ifndef KALEIDO_PODARRAY_H
#define KALEIDO_PODARRAY_H

#include <algorithm>
#include <cstddef>
#include <memory>
#include <numa.h>
#include <cstring>

#define likely(x) (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#define ALWAYS_INLINE __attribute__((__always_inline__))
#define NO_INLINE __attribute__((__noinline__))
#define MAY_ALIAS __attribute__((__may_alias__))

#ifdef __SSE2__

#include <emmintrin.h>

namespace detail {
    inline void memcpySmallAllowReadWriteOverflow15Impl(
            char *__restrict dst, const char *__restrict src, ssize_t n) {
        while (n > 0) {
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dst),
                             _mm_loadu_si128(reinterpret_cast<const __m128i *>(src)));

            dst += 16;
            src += 16;
            n -= 16;
        }
    }
}

inline void memcpySmallAllowReadWriteOverflow15(
        void *__restrict dst, const void *__restrict src, size_t n) {
    detail::memcpySmallAllowReadWriteOverflow15Impl(
            reinterpret_cast<char *>(dst), reinterpret_cast<const char *>(src), n);
}

#else /// Implementation for other platforms.

inline void memcpySmallAllowReadWriteOverflow15(
    void* __restrict dst, const void* __restrict src, size_t n) {
    memcpy(dst, src, n);
}

#endif

inline constexpr size_t integerRoundUp(size_t value, size_t dividend) {
    return ((value + dividend - 1) / dividend) * dividend;
}

inline constexpr size_t roundUpToPowerOfTwoOrZero(size_t n) {
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    ++n;

    return n;
}

static constexpr size_t EmptyPODArraySize = 1024;
extern const char EmptyPODArray[EmptyPODArraySize];

template<size_t ELEMENT_SIZE, size_t INITIAL_SIZE, size_t pad_right_, size_t pad_left_>
class PODArrayBase {
protected:
    /// Round padding up to an whole number of elements to simplify arithmetic.
    static constexpr size_t pad_right = integerRoundUp(pad_right_, ELEMENT_SIZE);
    /// pad_left is also rounded up to 16 bytes to maintain alignment of allocated memory.
    static constexpr size_t pad_left = integerRoundUp(integerRoundUp(pad_left_, ELEMENT_SIZE), 16);
    /// Empty array will point to this static memory as padding.
    static constexpr char *null
            = pad_left ? const_cast<char *>(EmptyPODArray) + EmptyPODArraySize : nullptr;

    static_assert(pad_left <= EmptyPODArraySize
                  && "Left Padding exceeds EmptyPODArraySize. Is the element size too large?");

    char *c_start = null; /// Does not include pad_left.
    char *c_end = null;
    char *c_end_of_storage = null; /// Does not include pad_right.
    int socket_id{};

    /// The amount of memory occupied by the num_elements of the elements.
    static size_t byte_size(size_t num_elements) { return num_elements * ELEMENT_SIZE; }

    /// Minimum amount of memory to allocate for num_elements, including padding.
    static size_t minimum_memory_for_elements(size_t num_elements) {
        return byte_size(num_elements) + pad_right + pad_left;
    }

    void alloc_for_num_elements(size_t num_elements) {
        alloc(roundUpToPowerOfTwoOrZero(minimum_memory_for_elements(num_elements)));
    }

    void alloc(size_t bytes) {
        auto x
                = socket_id == -1 ? numa_alloc_interleaved(bytes) : numa_alloc_onnode(bytes, socket_id);
        c_start = c_end = reinterpret_cast<char *>(x) + pad_left;
        c_end_of_storage = c_start + bytes - pad_right - pad_left;

        if (pad_left)
            memset(c_start - ELEMENT_SIZE, 0, ELEMENT_SIZE);
    }

    void dealloc() {
        if (c_start == null)
            return;

        numa_free(c_start - pad_left, allocated_bytes());
    }

    void realloc(size_t bytes) {
        if (c_start == null) {
            alloc(bytes);
            return;
        }
        ptrdiff_t end_diff = c_end - c_start;
        c_start
                = reinterpret_cast<char *>(numa_realloc(c_start - pad_left, allocated_bytes(), bytes))
                  + pad_left;
        c_end = c_start + end_diff;
        c_end_of_storage = c_start + bytes - pad_right - pad_left;
    }

    bool isInitialized() const {
        return (c_start != null) && (c_end != null) && (c_end_of_storage != null);
    }

    void reserveForNextSize() {
        if (size() == 0) {
            realloc(std::max(((INITIAL_SIZE - 1) / ELEMENT_SIZE + 1) * ELEMENT_SIZE,
                             minimum_memory_for_elements(1)));
        } else
            realloc(allocated_bytes() * 2);
    }

public:
#ifdef NUMA
    PODArrayBase(int socket_id = -1)
        : socket_id(socket_id) {}
#else

    PODArrayBase(int socket_id = -1)
            : socket_id(-1) {}

#endif

    bool empty() const { return c_end == c_start; }

    size_t size() const { return (c_end - c_start) / ELEMENT_SIZE; }

    size_t capacity() const { return (c_end_of_storage - c_start) / ELEMENT_SIZE; }

    size_t allocated_bytes() const { return c_end_of_storage - c_start + pad_right + pad_left; }

    void clear() { c_end = c_start; }

    void reserve(size_t n) {
        if (n > capacity())
            realloc(roundUpToPowerOfTwoOrZero(minimum_memory_for_elements(n)));
    }

    void resize(size_t n) {
        reserve(n);
        resize_assume_reserved(n);
    }

    void resize_assume_reserved(const size_t n) { c_end = c_start + byte_size(n); }

    const char *raw_data() const { return c_start; }

    void push_back_raw(const char *ptr) {
        if (unlikely(c_end == c_end_of_storage))
            reserveForNextSize();
        memcpy(c_end, ptr, ELEMENT_SIZE);
        c_end += byte_size(1);
    }

    ~PODArrayBase() { dealloc(); }
};

template<typename T, size_t INITIAL_SIZE = 4096, size_t pad_right_ = 0, size_t pad_left_ = 0>
class PODArray : public PODArrayBase<sizeof(T), INITIAL_SIZE, pad_right_, pad_left_> {
protected:
    using Base = PODArrayBase<sizeof(T), INITIAL_SIZE, pad_right_, pad_left_>;

    T *t_start() { return reinterpret_cast<T *>(this->c_start); }

    T *t_end() { return reinterpret_cast<T *>(this->c_end); }

    T *t_end_of_storage() { return reinterpret_cast<T *>(this->c_end_of_storage); }

    const T *t_start() const { return reinterpret_cast<const T *>(this->c_start); }

    const T *t_end() const { return reinterpret_cast<const T *>(this->c_end); }

    const T *t_end_of_storage() const { return reinterpret_cast<const T *>(this->c_end_of_storage); }

public:
    using value_type = T;

    /// You can not just use `typedef`, because there is ambiguity for the constructors and `assign`
    /// functions.
    using iterator = T *;
    using const_iterator = const T *;

    PODArray(int socket_id = -1)
            : Base(socket_id) {}

    PODArray(size_t n, int socket_id)
            : Base(socket_id) {
        this->alloc_for_num_elements(n);
        this->c_end += this->byte_size(n);
    }

    PODArray(const_iterator from_begin, const_iterator from_end) {
        this->alloc_for_num_elements(from_end - from_begin);
        insert(from_begin, from_end);
    }

    PODArray(std::initializer_list<T> il)
            : PODArray(std::begin(il), std::end(il)) {}

    PODArray(PODArray &&other) { this->swap(other); }

    PODArray &operator=(PODArray &&other) {
        this->swap(other);
        return *this;
    }

    PODArray clone() const {
        PODArray ret(this->size(), -1);
        ret.assign(*this);
        if (ret.size())
            ret[-1] = (*this)[-1];
        return ret;
    }

    T *data() { return t_start(); }

    const T *data() const { return t_start(); }

    /// The index is signed to access -1th element without pointer overflow.
    T &operator[](ssize_t n) { return t_start()[n]; }

    const T &operator[](ssize_t n) const { return t_start()[n]; }

    T &front() { return t_start()[0]; }

    T &back() { return t_end()[-1]; }

    const T &front() const { return t_start()[0]; }

    const T &back() const { return t_end()[-1]; }

    iterator begin() { return t_start(); }

    iterator end() { return t_end(); }

    const_iterator begin() const { return t_start(); }

    const_iterator end() const { return t_end(); }

    const_iterator cbegin() const { return t_start(); }

    const_iterator cend() const { return t_end(); }

    /// Same as resize, but zeroes new elements.
    void resize_fill(size_t n) {
        size_t old_size = this->size();
        if (n > old_size) {
            this->reserve(n);
            memset(this->c_end, 0, this->byte_size(n - old_size));
        }
        this->c_end = this->c_start + this->byte_size(n);
    }

    void resize_fill(size_t n, const T &value) {
        size_t old_size = this->size();
        if (n > old_size) {
            this->reserve(n);
            std::fill(t_end(), t_end() + n - old_size, value);
        }
        this->c_end = this->c_start + this->byte_size(n);
    }

    template<typename U>
    void push_back(U &&x) {
        if (unlikely(this->c_end == this->c_end_of_storage))
            this->reserveForNextSize();

        new(t_end()) T(std::forward<U>(x));
        this->c_end += this->byte_size(1);
    }

    template<typename... Args>
    void emplace_back(Args &&... args) {
        if (unlikely(this->c_end == this->c_end_of_storage))
            this->reserveForNextSize();

        new(t_end()) T(std::forward<Args>(args)...);
        this->c_end += this->byte_size(1);
    }

    void pop_back() { this->c_end -= this->byte_size(1); }

    /// Do not insert into the array a piece of itself. Because with the resize, the iterators on
    /// themselves can be invalidated.
    template<typename It1, typename It2>
    void insertPrepare(It1 from_begin, It2 from_end) {
        size_t required_capacity = this->size() + (from_end - from_begin);
        if (required_capacity > this->capacity())
            this->reserve(roundUpToPowerOfTwoOrZero(required_capacity));
    }

    /// Do not insert into the array a piece of itself. Because with the resize, the iterators on
    /// themselves can be invalidated.
    template<typename It1, typename It2>
    void insert(It1 from_begin, It2 from_end) {
        insertPrepare(from_begin, from_end);
        insert_assume_reserved(from_begin, from_end);
    }

    /// Works under assumption, that it's possible to read up to 15 excessive bytes after `from_end`
    /// and this PODArray is padded.
    template<typename It1, typename It2>
    void insertSmallAllowReadWriteOverflow15(It1 from_begin, It2 from_end) {
        static_assert(pad_right_ >= 15);
        insertPrepare(from_begin, from_end);
        size_t bytes_to_copy = this->byte_size(from_end - from_begin);
        memcpySmallAllowReadWriteOverflow15(
                this->c_end, reinterpret_cast<const void *>(&*from_begin), bytes_to_copy);
        this->c_end += bytes_to_copy;
    }

    template<typename It1, typename It2>
    void insert(iterator it, It1 from_begin, It2 from_end) {
        insertPrepare(from_begin, from_end);

        size_t bytes_to_copy = this->byte_size(from_end - from_begin);
        size_t bytes_to_move = (end() - it) * sizeof(T);

        if (unlikely(bytes_to_move))
            memcpy(this->c_end + bytes_to_copy - bytes_to_move, this->c_end - bytes_to_move,
                   bytes_to_move);

        memcpy(this->c_end - bytes_to_move, reinterpret_cast<const void *>(&*from_begin),
               bytes_to_copy);
        this->c_end += bytes_to_copy;
    }

    template<typename It1, typename It2>
    void insert_assume_reserved(It1 from_begin, It2 from_end) {
        size_t bytes_to_copy = this->byte_size(from_end - from_begin);
        memcpy(this->c_end, reinterpret_cast<const void *>(&*from_begin), bytes_to_copy);
        this->c_end += bytes_to_copy;
    }

    void swap(PODArray &rhs) {
        std::swap(this->socket_id, rhs.socket_id);
        if (!this->isInitialized() && !rhs.isInitialized())
            return;
//        else if (!this->isInitialized() && rhs.isInitialized()) {
        else {
            std::swap(this->c_start, rhs.c_start);
            std::swap(this->c_end, rhs.c_end);
            std::swap(this->c_end_of_storage, rhs.c_end_of_storage);
        }
    }

    void assign(size_t n, const T &x) {
        this->resize(n);
        std::fill(begin(), end(), x);
    }

    template<typename It1, typename It2>
    void assign(It1 from_begin, It2 from_end) {
        size_t required_capacity = from_end - from_begin;
        if (required_capacity > this->capacity())
            this->reserve(roundUpToPowerOfTwoOrZero(required_capacity));

        size_t bytes_to_copy = this->byte_size(required_capacity);
        memcpy(this->c_start, reinterpret_cast<const void *>(&*from_begin), bytes_to_copy);
        this->c_end = this->c_start + bytes_to_copy;
    }

    void assign(const PODArray &from) { assign(from.begin(), from.end()); }

    bool operator==(const PODArray &other) const {
        if (this->size() != other.size())
            return false;

        const_iterator this_it = begin();
        const_iterator that_it = other.begin();

        while (this_it != end()) {
            if (*this_it != *that_it)
                return false;

            ++this_it;
            ++that_it;
        }

        return true;
    }

    bool operator!=(const PODArray &other) const { return !operator==(other); }
};

template<typename T, size_t INITIAL_SIZE, size_t pad_right_>
void swap(PODArray<T, INITIAL_SIZE, pad_right_> &lhs, PODArray<T, INITIAL_SIZE, pad_right_> &rhs) {
    lhs.swap(rhs);
}

/** For columns. Padding is enough to read and write xmm-register at the address of the last
 * element. */
template<typename T, size_t INITIAL_SIZE = 4096> using Vector = PODArray<T, INITIAL_SIZE, 15, 16>;

#endif //KALEIDO_PODARRAY_H
