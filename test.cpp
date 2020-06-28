/** This is a test file for the memory pool implementation
 * It illustrates some basic usage of this memory resource
 */

#include <algorithm>
#include <bitset>
#include <random>
#include <vector>

#include "pool.hpp"

void push_random(std::vector<void *> &ptrs, mem::PoolMemory &pool, std::mt19937 &gen)
{
    std::uniform_int_distribution<int> dist(0, ptrs.size());
    auto index = dist(gen);
    ptrs.insert(ptrs.begin() + index, static_cast<void *>(pool.get()));
    std::cout << "Getting element: " << ptrs[index] << " from the memory pool" << std::endl;
    std::cout << "Currently we have " << pool.free_count() << " free element space" << std::endl;
    std::cout << "Is the memory pool full? " << (pool.full() ? "Yes" : "No") << std::endl;
}

void pop_random(std::vector<void *> &ptrs, mem::PoolMemory &pool, std::mt19937 &gen)
{
    std::uniform_int_distribution<int> dist(0, ptrs.size() - 1);
    auto index = dist(gen);
    std::cout << "Returning memory: " << ptrs[index] << " to the memory pool" << std::endl;
    pool.free(ptrs[index]);
    ptrs.erase(ptrs.begin() + index);
    std::cout << "Currently we have " << pool.free_count() << " free element space" << std::endl;
    std::cout << "Is the memory pool empty? " << (pool.empty() ? "Yes" : "No") << std::endl;
}

int main()
{
    int num_elements = 10;
    int bias = 5;
    int iter_num = 5;
    int actual_size;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<void *> ptrs;
    std::uniform_int_distribution<int> dist(num_elements - bias, num_elements + bias);
    std::uniform_int_distribution<int> tf(0, 1);

    using type = std::bitset<1024>;

    for (auto iteration = 0; iteration < iter_num; iteration++) {
        actual_size = dist(gen);
        std::cout << "Our actual size is: " << actual_size << std::endl;
        mem::PoolMemory pool(sizeof(type), actual_size);
        std::cout << "Initial size of this memory pool: " << pool.size() << std::endl;
        std::cout << "Initial capacity of this memory pool: " << pool.capacity() << std::endl;
        std::cout << "Initial free space of this memory pool: " << pool.free_count() << std::endl;
        std::cout << "Size of the memory pool variable: " << sizeof(pool) << std::endl;
        std::cout << "Size of the memory pool: " << pool.pool_size() << std::endl;
        ptrs.clear();

        for (auto i = 0; i < actual_size; i++) {
            push_random(ptrs, pool, gen);
        }

        pool.get();

        std::shuffle(ptrs.begin(), ptrs.end(), gen);

        for (auto i = 0; i < actual_size; i++) {
            pop_random(ptrs, pool, gen);
        }

        // std::cout << "[TEST] Random number is: " << tf(gen) << std::endl;

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
        if (!ptrs.empty()) {
            std::size_t size = ptrs.size();
            for (auto i = 0; i < size; i++) {
                std::cout << "Popping since not empty yet, index is: " << i << " size is: " << ptrs.size() << std::endl;
                pop_random(ptrs, pool, gen);
            }
        }
    }

    std::cout << "Size of a type is: " << sizeof(type) << std::endl;
    std::cout << "Size of a bitset<128> is: " << sizeof(std::bitset<128>) << std::endl;
    std::cout << "Size of a std::size_t is: " << sizeof(std::size_t) << std::endl;
    std::cout << "Size of a void * is: " << sizeof(void *) << std::endl;
    std::cout << "Size of a bool is: " << sizeof(bool) << std::endl;
    std::cout << "Size of a PoolMemory is: " << sizeof(mem::PoolMemory) << std::endl;
    std::cout << "Size of a ByteMemory is: " << sizeof(mem::ByteMemory) << std::endl;
    std::cout << "Test is completed, bye." << std::endl;
}