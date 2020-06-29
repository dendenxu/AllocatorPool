/**
 * This is a test file for the memory pool implementation
 * It illustrates some basic usage of this memory resource
 */

#include <algorithm>  // to shuffle vector
#include <bitset>     // to create arbitrarily sized type
#include <chrono>     // to use high resolution clock
#include <random>     // to use random generator and random devices
#include <ratio>      // to use with chrono
#include <vector>

#include "pool.hpp"
/* clang-format off */
// #define VERBOSE  // whether we're to silent everybody
#define TEST_POOL  // are we test pool memory resource?
#define TEST_MONO  // are we test monotonic memory resource?
/* clang-format on */

using hiclock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<hiclock>;
using duration = std::chrono::duration<double>;
using std::chrono::duration_cast;
constexpr int scale = 16;               // scale of our test, 2^scale
constexpr int num_blocks = 2 << scale;  // base of number of blocks, actual possible range: [num_blocks-bias, num_blocks+bias]
constexpr int bias = num_blocks / 2;    // the bias to be added to base number, range: [num_blocks-bias, num_blocks+bias]
constexpr int num_iters = 5;            // number of iteration to test, each with a newly allocated PoolMemory and random block count
using type = std::bitset<1024>;         // we can change this type to test for different size of allocation
// using type = int;                     // should produce assertion failure for pool memory, on my machine sizeof(int) == 4
// using type = double;                  // should produce a densely used memory pool, on my machine sizeof(double) == 8 == sizeof(void *)

/** Print some information about the current memory resource, assuming free_count, capacity and full, empty API */
template <class MemoT>
void print_info(MemoT &memo)
{
    std::cout
        << "Currently we have "
        << memo.free_count()
        << " free block and totally "
        << memo.capacity()
        << " blocks"
        << std::endl;
    std::cout << "Is the memory resource full? " << (memo.full() ? "Yes" : "No") << std::endl;
    std::cout << "Is the memory resource empty? " << (memo.empty() ? "Yes" : "No") << std::endl;
}

/** Get a memory pointer from the given memory resource and insert it to the back (or a given position) of the given pointer vector */
void push(std::vector<void *> &ptrs, mem::PoolMemory &pool, duration &span, std::size_t index = -1)
{
    if (index == -1) index = ptrs.size();
    auto begin = hiclock::now();
    auto pmem = pool.get();
    auto end = hiclock::now();
    span += duration_cast<duration>(end - begin);
    ptrs.insert(ptrs.begin() + index, static_cast<void *>(pmem));
#ifdef VERBOSE
    std::cout << "Getting block: " << ptrs[index] << " from the memory resource" << std::endl;
    print_info(pool);
#endif  // VERBOSE
}

/** Select last (or a given position) pointer of a given pointer vector and give it back to the given memory resource */
void pop(std::vector<void *> &ptrs, mem::PoolMemory &pool, duration &span, std::size_t index = -1)
{
    if (index == -1) index = ptrs.size() - 1;
#ifdef VERBOSE
    std::cout << "Returning block: " << ptrs[index] << " to the memory resource" << std::endl;
#endif  // VERBOSE

    auto begin = hiclock::now();
    pool.free(ptrs[index]);
    auto end = hiclock::now();
    span += duration_cast<duration>(end - begin);
    ptrs.erase(ptrs.begin() + index);
#ifdef VERBOSE
    print_info(pool);
#endif  // VERBOSE
}

/** Get a memory pointer from the given memory resource and insert it to the back of the given pointer vector */
template <class MemoT, class VectT>
void push(VectT &ptrs_with_sz, MemoT &memo, std::mt19937 &gen, duration &span, std::size_t index = -1)
{
    if (index == -1) index = ptrs_with_sz.size();
    std::uniform_int_distribution<std::size_t> dist(0, memo.free_count());
    auto size = dist(gen);
    auto begin = hiclock::now();
    auto pmem = memo.get(size);
    auto end = hiclock::now();
    span += duration_cast<duration>(end - begin);
    ptrs_with_sz.insert(ptrs_with_sz.begin() + index, std::make_pair(pmem, size));
#ifdef VERBOSE
    std::cout << "Getting block: " << pmem << " from the memory resource with size: " << size << std::endl;
    print_info(memo);
#endif  // VERBOSE
}

/** Select last pointer of a given pointer vector and give it back to the given memory resource */
template <class MemoT, class VectT>
void pop(VectT &ptrs_with_sz, MemoT &memo, duration &span, std::size_t index = -1)
{
    if (index == -1) index = ptrs_with_sz.size() - 1;
    auto pair = ptrs_with_sz[index];
#ifdef VERBOSE
    std::cout << "Returning block: " << pair.first << " to the memory resource with size " << pair.second << std::endl;
#endif  // VERBOSE

    ptrs_with_sz.erase(ptrs_with_sz.begin() + index);
    auto begin = hiclock::now();
    memo.free(pair.first, pair.second);
    auto end = hiclock::now();
    span += duration_cast<duration>(end - begin);
#ifdef VERBOSE
    print_info(memo);
#endif  // VERBOSE
}

/** Get a memory pointer from the given memory pool and insert it to a random position of the given pointer vector */
template <class MemoT, class VectT>
void push_random(VectT &ptrs_with_sz, MemoT &memo, std::mt19937 &gen, duration &span)
{
    std::uniform_int_distribution<std::size_t> dist(0, ptrs_with_sz.size());  // we can insert at [0, ptrs.size()]
    std::size_t index = dist(gen);
    push(ptrs_with_sz, memo, gen, span, index);
}

/** Select a random position from a given pointer vector and give it back to the given memory resource */
template <class MemoT, class VectT>
void pop_random(VectT &ptrs_with_sz, MemoT &memo, std::mt19937 &gen, duration &span)
{
    std::uniform_int_distribution<std::size_t> dist(0, ptrs_with_sz.size() - 1);  // we can erase at [0, ptrs.size()), half-closed range
    std::size_t index = dist(gen);
    pop(ptrs_with_sz, memo, span, index);
}

/** Get a memory pointer from the given memory pool and insert it to a random position of the given pointer vector */
void push_random(std::vector<void *> &ptrs, mem::PoolMemory &pool, std::mt19937 &gen, duration &span)
{
    std::uniform_int_distribution<std::size_t> dist(0, ptrs.size());  // we can insert at [0, ptrs.size()]
    std::size_t index = dist(gen);
    push(ptrs, pool, span, index);
}

/** Select a random position from a given pointer vector and give it back to the given memory resource */
void pop_random(std::vector<void *> &ptrs, mem::PoolMemory &pool, std::mt19937 &gen, duration &span)
{
    std::uniform_int_distribution<std::size_t> dist(0, ptrs.size() - 1);  // we can erase at [0, ptrs.size()), half-closed range
    std::size_t index = dist(gen);
    pop(ptrs, pool, span, index);
}

int main()
{
    int actual_size;           // reused in every iteration, range in [num_blocks-bias, num_blocks+bias]
    std::random_device rd;     // depends on the current system, increase entropy of random gen, heavy: involving file IO
    std::mt19937 gen(rd());    // a popular random number generator
    std::vector<void *> ptrs;  // the vector of pointers to be cleared and reused in every iterations
    std::vector<               // the vector of pointers along with their size
        std::pair<
            void *,
            std::size_t>>
        ptrs_with_sz;

    std::uniform_int_distribution<std::size_t>
        dist(num_blocks - bias, num_blocks + bias);       // distribution to generate actual size
    std::uniform_int_distribution<bool> tf(false, true);  // true false binary random generator
    duration span = duration();                           // globally used time duration

    time_point begin;  // used in chrono timing
    time_point end;    // used in chrono timing

#ifdef TEST_MONO
    for (auto iteration = 0; iteration < num_iters; iteration++) {
        actual_size = dist(gen);  // range: [num_blocks-bias, num_blocks+bias]
        mem::MonoMemory *pmono = nullptr;
        std::byte *ptr = nullptr;

        if (tf(gen)) {  // the string would be already quite self-explaining
            std::cout << "[INFO] We're doing the allocation manually" << std::endl;
            begin = hiclock::now();
            pmono = new mem::MonoMemory(sizeof(type) * actual_size);
            end = hiclock::now();
        } else {
            std::cout << "[INFO] We're doing the allocation ahead of time" << std::endl;
            ptr = new std::byte[actual_size];  // the deletion is at the end of the iteration
            begin = hiclock::now();
            pmono = new mem::MonoMemory(sizeof(type) * actual_size, ptr);
            end = hiclock::now();
        }

        mem::MonoMemory &mono = *pmono;

        std::cout
            << "It takes "
            << duration_cast<duration>(end - begin).count()
            << " seconds to create and initialize the byte memory resource"
            << std::endl;

        /** Print some auxiliary information */
        std::cout << "Our actual size is: " << actual_size << std::endl;
        std::cout << "Initial size of this byte memory: " << mono.size() << std::endl;
        std::cout << "Initial capacity of this byte memory: " << mono.capacity() << std::endl;
        std::cout << "Initial free space of this byte memory: " << mono.free_count() << std::endl;
        std::cout << "Size of the byte memory variable: " << sizeof(mono) << std::endl;
        std::cout << "Size of the byte memory: " << mono.capacity() << std::endl;

        ptrs_with_sz.clear();
        auto count = 0;
        span = duration();
        while (!mono.full()) {
            count++;
            push(ptrs_with_sz, mono, gen, span);
        }
        std::cout << "Is the memory resource full? " << (mono.full() ? "Yes" : "No") << std::endl;

        std::cout
            << "It takes "
            << span.count()
            << " seconds to get this much times, which averages to "
            << span.count() / count
            << " seconds per operations"
            << std::endl;

        span = duration();

        auto size = ptrs_with_sz.size();
        for (auto i = 0; i < size; i++) {
            pop(ptrs_with_sz, mono, span);
        }

        std::cout
            << "It takes "
            << span.count()
            << " seconds to free this much times, which averages to "
            << span.count() / size
            << " seconds per operations"
            << std::endl;

        /** Test for some random allocation and deallocation requests, mixing up the order */
        for (auto i = 0; i < actual_size; i++) {
            if (tf(gen)) {
                if (mono.full()) {
                    pop(ptrs_with_sz, mono, span);
                } else {
                    push(ptrs_with_sz, mono, gen, span);
                }
            } else {
                if (mono.empty()) {
                    push(ptrs_with_sz, mono, gen, span);
                } else {
                    pop(ptrs_with_sz, mono, span);
                }
            }
        }

        std::cout << "The pool's current size: " << mono.size() << std::endl;
        std::cout << "The ptrs's current size: " << ptrs_with_sz.size() << std::endl;

        /** Empty the whole memory pool if it's not currently empty */
        if (!ptrs_with_sz.empty()) {
            std::size_t size = ptrs_with_sz.size();  // we should memorize this since the size is changed every time we call pop or push
            for (auto i = 0; i < size; i++) {
#ifdef VERBOSE
                std::cout << "Popping since not empty yet, index is: " << i << " size is: " << ptrs_with_sz.size() << std::endl;
#endif  // VERBOSE
                pop(ptrs_with_sz, mono, span);
            }
        }

        delete pmono;
        delete[] ptr;  // we might be deleting a nullptr
    }
#endif  // TEST_MONO

#ifdef TEST_POOL
    for (auto iteration = 0; iteration < num_iters; iteration++) {
        actual_size = dist(gen);  // range: [num_blocks-bias, num_blocks+bias]

        mem::PoolMemory *ppool = nullptr;
        std::byte *ptr = nullptr;
        if (tf(gen)) {
            std::cout << "[INFO] We're doing the allocation manually" << std::endl;
            begin = hiclock::now();
            ppool = new mem::PoolMemory(sizeof(type), actual_size);
            end = hiclock::now();
        } else {
            std::cout << "[INFO] We're doing the allocation ahead of time" << std::endl;
            ptr = new std::byte[actual_size * sizeof(type)];
            begin = hiclock::now();
            ppool = new mem::PoolMemory(sizeof(type), actual_size, ptr);
            end = hiclock::now();
        }

        mem::PoolMemory &pool = *ppool;

        std::cout
            << "It takes "
            << duration_cast<duration>(end - begin).count()
            << " seconds to create and initialize the memory pool"
            << std::endl;

        /** Print some auxiliary information */
        std::cout << "Our actual size is: " << actual_size << std::endl;
        std::cout << "Initial size of this memory pool: " << pool.size() << std::endl;
        std::cout << "Initial capacity of this memory pool: " << pool.capacity() << std::endl;
        std::cout << "Initial free space of this memory pool: " << pool.free_count() << std::endl;
        std::cout << "Size of the memory pool variable: " << sizeof(pool) << std::endl;
        std::cout << "Size of the memory pool: " << pool.pool_size() << std::endl;

        ptrs.clear();  // clear pointer vector on every iteration

        /** Exhaust all the memory available in the memory pool */
        span = duration();
        for (auto i = 0; i < actual_size; i++) {
            push_random(ptrs, pool, gen, span);
        }
        std::cout
            << "It takes "
            << span.count()
            << " seconds to get this much times, which averages to "
            << span.count() / actual_size
            << " seconds per operations"
            << std::endl;

        try {
            pool.get();  // should throw a bad_alloc
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
        }

        /** Returning all the memory exhausted before */
        span = duration();
        for (auto i = 0; i < actual_size; i++) {
            pop_random(ptrs, pool, gen, span);
        }
        std::cout
            << "It takes "
            << span.count()
            << " seconds to free this much times, which averages to "
            << span.count() / actual_size
            << " seconds per operations"
            << std::endl;

        /** Test for some random allocation and deallocation requests, mixing up the order */
        for (auto i = 0; i < actual_size; i++) {
            if (tf(gen)) {
                if (pool.full()) {
                    pop_random(ptrs, pool, gen, span);
                } else {
                    push_random(ptrs, pool, gen, span);
                }
            } else {
                if (pool.empty()) {
                    push_random(ptrs, pool, gen, span);
                } else {
                    pop_random(ptrs, pool, gen, span);
                }
            }
        }

        std::cout << "The pool's current size: " << pool.size() << std::endl;
        std::cout << "The ptrs's current size: " << ptrs.size() << std::endl;

        /** Empty the whole memory pool if it's not currently empty */
        if (!ptrs.empty()) {
            std::size_t size = ptrs.size();  // we should memorize this since the size is changed every time we call pop or push
            for (auto i = 0; i < size; i++) {
#ifdef VERBOSE
                std::cout << "Popping since not empty yet, index is: " << i << " size is: " << ptrs.size() << std::endl;
#endif  // VERBOSE

                pop_random(ptrs, pool, gen, span);
            }
        }

        std::cout << "Is the memory pool eventually empty? " << (pool.empty() ? "Yes" : "No") << std::endl;

        delete ppool;
        delete[] ptr;  // we might be deleting a nullptr
    }
#endif  // TEST_POOL

    /** Print more auxiliary information */
    std::cout << "Size of a type is: " << sizeof(type) << std::endl;
    std::cout << "Size of a bitset<128> is: " << sizeof(std::bitset<128>) << std::endl;
    std::cout << "Size of a std::size_t is: " << sizeof(std::size_t) << std::endl;
    std::cout << "Size of a void * is: " << sizeof(void *) << std::endl;
    std::cout << "Size of a bool is: " << sizeof(bool) << std::endl;
    std::cout << "Size of a PoolMemory is: " << sizeof(mem::PoolMemory) << std::endl;
    std::cout << "Size of a MonoMemory is: " << sizeof(mem::MonoMemory) << std::endl;
    std::cout << "Test is completed, bye." << std::endl;
}