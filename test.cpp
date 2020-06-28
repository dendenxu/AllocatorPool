/**
 * This is a test file for the memory pool implementation
 * It illustrates some basic usage of this memory resource
 */

#include <algorithm>  // to shuffle vector
#include <bitset>     // to create arbitrarily sized type
#include <chrono>     // to use high resolution clock
#include <random>     // to use random generator and random devices
#include <vector>

#include "pool.hpp"
using hiclock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<hiclock>;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::seconds;
using type = std::bitset<1024>;  // we can change this type to test for different size of allocation
/** Get a memory pointer from the given memory pool and insert it to a random position of the given pointer vector */
void push_random(std::vector<void *> &ptrs, mem::PoolMemory &pool, std::mt19937 &gen, duration<double> &span, bool silent = false)
{
    std::uniform_int_distribution<std::size_t> dist(0, ptrs.size());  // we can insert at [0, ptrs.size()]
    std::size_t index = dist(gen);
    auto begin = hiclock::now();
    auto pmem = pool.get();
    auto end = hiclock::now();
    span += duration_cast<duration<double>>(end - begin);
    ptrs.insert(ptrs.begin() + index, static_cast<void *>(pmem));
    if (!silent) {
        std::cout << "Getting block: " << ptrs[index] << " from the memory pool" << std::endl;
        std::cout
            << "Currently we have "
            << pool.free_count()
            << " free block and totally "
            << pool.capacity()
            << " blocks"
            << std::endl;
        std::cout << "Is the memory pool full? " << (pool.full() ? "Yes" : "No") << std::endl;
    }
}

/** Select a random position from a given pointer vector and give it back to the given memory pool */
void pop_random(std::vector<void *> &ptrs, mem::PoolMemory &pool, std::mt19937 &gen, duration<double> &span, bool silent = false)
{
    std::uniform_int_distribution<std::size_t> dist(0, ptrs.size() - 1);  // we can erase at [0, ptrs.size()), half-closed range
    std::size_t index = dist(gen);
    if (!silent) {
        std::cout << "Returning block: " << ptrs[index] << " to the memory pool" << std::endl;
    }
    auto begin = hiclock::now();
    pool.free(ptrs[index]);
    auto end = hiclock::now();
    span += duration_cast<duration<double>>(end - begin);
    ptrs.erase(ptrs.begin() + index);
    if (!silent) {
        std::cout
            << "Currently we have "
            << pool.free_count()
            << " free block and totally "
            << pool.capacity()
            << " blocks"
            << std::endl;
        std::cout << "Is the memory pool empty? " << (pool.empty() ? "Yes" : "No") << std::endl;
    }
}

int main()
{
    bool silent = true;        // are we silencing output?
    int num_blocks = 100000;   // base of number of blocks, actual possible range: [num_blocks-bias, num_blocks+bias]
    int bias = 5;              // the bias to be added to base number, range: [num_blocks-bias, num_blocks+bias]
    int num_iters = 5;         // number of iteration to test, each with a newly allocated PoolMemory and random block count
    int actual_size;           // reused in every iteration, range in [num_blocks-bias, num_blocks+bias]
    std::random_device rd;     // a random device, depends on the current system, increase entropy of random gen, heavy: involving file IO
    std::mt19937 gen(rd());    // a popular random number generator
    std::vector<void *> ptrs;  // the vector of pointers to be cleared and reused in every iterations
    std::uniform_int_distribution<std::size_t> dist(num_blocks - bias, num_blocks + bias);
    std::uniform_int_distribution<bool> tf(false, true);
    duration<double> span = seconds::zero();
    // using type = int;  // should produce assertion failure, on my machine sizeof(int) == 4
    // using type = int;  // should produce a densely used memory pool, on my machine sizeof(double) == 8 == sizeof(void *)

    for (auto iteration = 0; iteration < num_iters; iteration++) {
        actual_size = dist(gen);  // range: [num_blocks-bias, num_blocks+bias]

        time_point begin = hiclock::now();
        mem::PoolMemory pool(sizeof(type), actual_size);
        time_point end = hiclock::now();

        std::cout
            << "It takes "
            << duration_cast<duration<double>>(end - begin).count()
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

        // begin = hiclock::now();
        /** Exhaust all the memory available in the memory pool */
        span = seconds::zero();
        for (auto i = 0; i < actual_size; i++) {
            push_random(ptrs, pool, gen, span, silent);
        }
        // end = hiclock::now();
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

        // std::shuffle(ptrs.begin(), ptrs.end(), gen); // might not be needed anymore since our push/pop is random

        /** Returning all the memory exhausted before */
        span = seconds::zero();
        for (auto i = 0; i < actual_size; i++) {
            pop_random(ptrs, pool, gen, span, silent);
        }
        std::cout
            << "It takes "
            << span.count()
            << " seconds to free this much times, which averages to "
            << span.count() / actual_size
            << " seconds per operations"
            << std::endl;

        // std::cout << "[TEST] Random number is: " << tf(gen) << std::endl;

        /** Test for some random allocation and deallocation requests, mixing up the order */
        for (auto i = 0; i < actual_size; i++) {
            if (tf(gen)) {
                if (pool.full()) {
                    pop_random(ptrs, pool, gen, span, silent);
                } else {
                    push_random(ptrs, pool, gen, span, silent);
                }
            } else {
                if (pool.empty()) {
                    push_random(ptrs, pool, gen, span, silent);
                } else {
                    pop_random(ptrs, pool, gen, span, silent);
                }
            }
            // std::shuffle(ptrs.begin(), ptrs.end(), gen); // ! this might be some heavy work
        }

        std::cout << "The pool's current size: " << pool.size() << std::endl;
        std::cout << "The ptrs's current size: " << ptrs.size() << std::endl;

        /** Empty the whole memory pool if it's not currently empty */
        if (!ptrs.empty()) {
            std::size_t size = ptrs.size();  // we should memorize this since the size is changed every time we call pop or push
            for (auto i = 0; i < size; i++) {
                if (!silent) {
                    std::cout << "Popping since not empty yet, index is: " << i << " size is: " << ptrs.size() << std::endl;
                }
                pop_random(ptrs, pool, gen, span, silent);
            }
        }
    }

    /** Print more auxiliary information */
    std::cout << "Size of a type is: " << sizeof(type) << std::endl;
    std::cout << "Size of a bitset<128> is: " << sizeof(std::bitset<128>) << std::endl;
    std::cout << "Size of a std::size_t is: " << sizeof(std::size_t) << std::endl;
    std::cout << "Size of a void * is: " << sizeof(void *) << std::endl;
    std::cout << "Size of a bool is: " << sizeof(bool) << std::endl;
    std::cout << "Size of a PoolMemory is: " << sizeof(mem::PoolMemory) << std::endl;
    std::cout << "Size of a ByteMemory is: " << sizeof(mem::ByteMemory) << std::endl;
    std::cout << "Test is completed, bye." << std::endl;
}