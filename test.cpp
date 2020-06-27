/** This is a test file for the memory pool implementation
 * It illustrates some basic usage of this memory resource
 */

#include <algorithm>
#include <random>
#include <vector>

#include "pool.hpp"

int main()
{
    int num_elements = 10;
    int bias = 5;
    int iter_num = 5;
    int actual_size;
    using type = double;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::vector<type *> ptrs;
    std::uniform_int_distribution<int> dist(num_elements - bias, num_elements + bias);

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
            ptrs.push_back(static_cast<type *>(pool.get()));
            std::cout << "Getting element: " << ptrs.back() << " from the memory pool" << std::endl;
            std::cout << "Currently we have " << pool.free_count() << " free element space" << std::endl;
            std::cout << "Is the memory pool full? " << (pool.full() ? "Yes." : "No.") << std::endl;
        }

        pool.get();

        std::shuffle(ptrs.begin(), ptrs.end(), gen);

        for (auto &ptr : ptrs) {
            std::cout << "Returning memory: " << ptr << " to the memory pool" << std::endl;
            pool.free(ptr);
            std::cout << "Currently we have " << pool.free_count() << " free element space" << std::endl;
        }
    }
}