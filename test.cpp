/** This is a test file for the memory pool implementation
 * It illustrates some basic usage of this memory resource
 */

#include <algorithm>
#include <random>
#include <vector>

#include "pool.hpp"
using type = double;
std::random_device rd;
std::mt19937 gen(rd());
void push(std::vector<type *> &ptrs, mem::PoolMemory &pool)
{
    std::uniform_int_distribution<int> dist(0, ptrs.size());
    auto index = dist(gen);
    ptrs.insert(ptrs.begin() + index, static_cast<type *>(pool.get()));
    std::cout << "Getting element: " << ptrs[index] << " from the memory pool" << std::endl;
    std::cout << "Currently we have " << pool.free_count() << " free element space" << std::endl;
    std::cout << "Is the memory pool full? " << (pool.full() ? "Yes." : "No.") << std::endl;
}

void pop(std::vector<type *> &ptrs, mem::PoolMemory &pool)
{
    std::uniform_int_distribution<int> dist(0, ptrs.size() - 1);
    auto index = dist(gen);
    std::cout << "Returning memory: " << ptrs[index] << " to the memory pool" << std::endl;
    pool.free(ptrs[index]);
    ptrs.erase(ptrs.begin() + index);
    std::cout << "Currently we have " << pool.free_count() << " free element space" << std::endl;
    std::cout << "Is the memory pool empty? " << (pool.empty() ? "Yes." : "No.") << std::endl;
}

int main()
{
    int num_elements = 10;
    int bias = 5;
    int iter_num = 5;
    int actual_size;
    std::vector<type *> ptrs;
    std::uniform_int_distribution<int> dist(num_elements - bias, num_elements + bias);
    std::uniform_int_distribution<int> tf(0, 1);

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
            push(ptrs, pool);
        }

        pool.get();

        std::shuffle(ptrs.begin(), ptrs.end(), gen);

        for (auto i = 0; i < actual_size; i++) {
            pop(ptrs, pool);
        }

        // std::cout << "[TEST] Random number is: " << tf(gen) << std::endl;

        for (auto i = 0; i < actual_size; i++) {
            if (tf(gen)) {
                if (pool.full()) {
                    pop(ptrs, pool);
                } else {
                    push(ptrs, pool);
                }
            } else {
                if (pool.empty()) {
                    push(ptrs, pool);
                } else {
                    pop(ptrs, pool);
                }
            }
            // std::shuffle(ptrs.begin(), ptrs.end(), gen); // ! this might be some heavy work
        }
    }

    std::cout << "Test is completed, bye." << std::endl;
}