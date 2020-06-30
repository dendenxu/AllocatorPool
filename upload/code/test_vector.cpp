#include "myAllocator.hpp"
#include <iostream>
#include <memory>
#include <vector>
#include <random>
#include <chrono>
#include <ratio>

const int TestSize = 20000;     // total size for test vector.
const int PickSize = 2000;      // the number to be randomly chosen from testsize.

template <class T>
using MyAllocator = vector::allocator<T>;  // replace the std::allocator with my allocator.
using Point2D = std::pair<int, int>;
using type_name = Point2D;
using T_Vec = std::vector<type_name, MyAllocator<type_name>>;

using hiclock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<hiclock>;
using duration = std::chrono::duration<double>;
using std::chrono::duration_cast;

time_point a_begin, a_end;

int main() {
    
    std::cout << "------------ Test case for MyAllocator of vector ------------" << std::endl;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, TestSize);

    time_point begin, end;

    a_begin = hiclock::now();
    begin = hiclock::now();
    std::vector<T_Vec, MyAllocator<T_Vec>> vecdous(TestSize);
    end = hiclock::now();

    std::cout 
        << "It takes " 
        << duration_cast<duration>(end - begin).count() 
        << " seconds to create this vector" 
        <<std::endl;

    // allocate to testsize vectors.`
    begin = hiclock::now();
    for (int i = 0; i < TestSize; i++) {
        vecdous[i].resize(dis(gen));
    }

    end = hiclock::now();

    std::cout
        << "It takes "
        << duration_cast<duration>(end - begin).count()
        << " seconds to resize "
        << TestSize
        << " vectors"
        << std::endl;

    // vector resize randomly.
    begin = hiclock::now();
    for (int i = 0; i < PickSize; i++) {
        int idx = dis(gen) - 1;
        int size = dis(gen);
        vecdous[idx].resize(size);
    }
    // vector shrink randomly
    for (int i = 0; i < PickSize / 2; i++) {
        int idx = dis(gen) - 1;
        int size = dis(gen);
        vecdous[idx].shrink_to_fit();
    }
    end = hiclock::now();

    std::cout
        << "It takes "
        << duration_cast<duration>(end - begin).count()
        << " seconds to resize "
        << PickSize
        << " picked vectors"
        << std::endl;

    // test if we can make correct assignment.
    {
        type_name val(11, 15);
        int idx1 = dis(gen) - 1;
        int idx2 = vecdous[idx1].size() / 2;
        vecdous[idx1][idx2] = val;
        if (vecdous[idx1][idx2] == val)
            std::cout << "correct assignment in vecdous: " << idx1 << std::endl;
        else
            std::cout << "incorrect assignment in vecdous: " << idx1 << std::endl;
    }
    a_end = hiclock::now();
    std::cout
        << "It takes "
        << duration_cast<duration>(a_end - a_begin).count()
        << " seconds in total"
        << std::endl;
    return 0;
}