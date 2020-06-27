#include "pool.hpp"

int main()
{
    int num_elements = 10;
    mem::PoolMemory pool(sizeof(double), num_elements);
    std::vector<double *> ptrs;
    for (auto i = 0; i < num_elements; i++) {
        ptrs.push_back(static_cast<double *>(pool.get()));
        std::cout << "Getting element: " << ptrs.back() << " from the memory pool" << std::endl;
        std::cout << "Currently we have " << pool.free_list_size() << " free element space" << std::endl;
        std::cout << "Is the memory pool empty? " << (pool.empty() ? "Yes." : "No.") << std::endl;
    }
    pool.get();
    for (auto &ptr : ptrs) {
        std::cout << "Returning memory: " << ptr << " to the memory pool" << std::endl;
        pool.free(ptr);
        std::cout << "Currently we have " << pool.free_list_size() << " free element space" << std::endl;
    }
}