/**
 * This is a test file for the memory pool implementation
 * It illustrates some basic usage of this memory resource
 */

#include <algorithm>  // to shuffle vector
#include <bitset>     // to create arbitrarily sized type
#include <random>     // to use random generator and random devices
#include <vector>

#include "pool.hpp"

/** Get a memory pointer from the given memory pool and insert it to a random position of the given pointer vector */
void push_random(std::vector<void *> &ptrs, mem::PoolMemory &pool, std::mt19937 &gen)
{
    std::uniform_int_distribution<std::size_t> dist(0, ptrs.size());  // we can insert at [0, ptrs.size()]
    std::size_t index = dist(gen);
    ptrs.insert(ptrs.begin() + index, static_cast<void *>(pool.get()));
    std::cout << "Getting block: " << ptrs[index] << " from the memory pool" << std::endl;
    std::cout << "Currently we have " << pool.free_count() << " free block and totally " << pool.capacity() << " blocks" << std::endl;
    std::cout << "Is the memory pool full? " << (pool.full() ? "Yes" : "No") << std::endl;
}

/** Select a random position from a given pointer vector and give it back to the given memory pool */
void pop_random(std::vector<void *> &ptrs, mem::PoolMemory &pool, std::mt19937 &gen)
{
    std::uniform_int_distribution<std::size_t> dist(0, ptrs.size() - 1);  // we can erase at [0, ptrs.size()), half-closed range
    std::size_t index = dist(gen);
    std::cout << "Returning block: " << ptrs[index] << " to the memory pool" << std::endl;
    pool.free(ptrs[index]);
    ptrs.erase(ptrs.begin() + index);
    std::cout << "Currently we have " << pool.free_count() << " free block and totally " << pool.capacity() << " blocks" << std::endl;
    std::cout << "Is the memory pool empty? " << (pool.empty() ? "Yes" : "No") << std::endl;
}

int main()
{
    int num_blocks = 10;       // base of number of blocks, actual possible range: [num_blocks-bias, num_blocks+bias]
    int bias = 5;              // the bias to be added to base number, range: [num_blocks-bias, num_blocks+bias]
    int num_iters = 5;         // number of iteration to test, each with a newly allocated PoolMemory and random block count
    int actual_size;           // reused in every iteration, range in [num_blocks-bias, num_blocks+bias]
    std::random_device rd;     // a random device, depends on the current system, increase entropy of random gen, heavy: involving file IO
    std::mt19937 gen(rd());    // a popular random number generator
    std::vector<void *> ptrs;  // the vector of pointers to be cleared and reused in every iterations
    std::uniform_int_distribution<std::size_t> dist(num_blocks - bias, num_blocks + bias);
    std::uniform_int_distribution<bool> tf(false, true);

    using type = std::bitset<1024>;  // we can change this type to test for different size of allocation
    // using type = int;  // should produce assertion failure, on my machine sizeof(int) == 4
    // using type = int;  // should produce a densely used memory pool, on my machine sizeof(double) == 8 == sizeof(void *)

    for (auto iteration = 0; iteration < num_iters; iteration++) {
        actual_size = dist(gen);  // range: [num_blocks-bias, num_blocks+bias]
        mem::PoolMemory pool(sizeof(type), actual_size);

        /** Print some auxiliary information */
        std::cout << "Our actual size is: " << actual_size << std::endl;
        std::cout << "Initial size of this memory pool: " << pool.size() << std::endl;
        std::cout << "Initial capacity of this memory pool: " << pool.capacity() << std::endl;
        std::cout << "Initial free space of this memory pool: " << pool.free_count() << std::endl;
        std::cout << "Size of the memory pool variable: " << sizeof(pool) << std::endl;
        std::cout << "Size of the memory pool: " << pool.pool_size() << std::endl;

        ptrs.clear();  // clear pointer vector on every iteration

        /** Exhaust all the memory available in the memory pool */
        for (auto i = 0; i < actual_size; i++) {
            push_random(ptrs, pool, gen);
        }

        try {
            pool.get();  // should throw a bad_alloc
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
        }

        // std::shuffle(ptrs.begin(), ptrs.end(), gen); // might not be needed anymore since our push/pop is random

        /** Returning all the memory exhausted before */
        for (auto i = 0; i < actual_size; i++) {
            pop_random(ptrs, pool, gen);
        }

        // std::cout << "[TEST] Random number is: " << tf(gen) << std::endl;

        /** Test for some random allocation and deallocation requests, mixing up the order */
        for (auto i = 0; i < actual_size; i++) {
            if (tf(gen)) {
                if (pool.full()) {
                    pop_random(ptrs, pool, gen);
                } else {
                    push_random(ptrs, pool, gen);
                }
            } else {
                if (pool.empty()) {
                    push_random(ptrs, pool, gen);
                } else {
                    pop_random(ptrs, pool, gen);
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
                std::cout << "Popping since not empty yet, index is: " << i << " size is: " << ptrs.size() << std::endl;
                pop_random(ptrs, pool, gen);
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