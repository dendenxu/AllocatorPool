#include "myAllocator.hpp"
#include <iostream>
#include <memory>
#include <list>
#include <random>
#include <chrono>
#include <ratio>

const int TestSize = 20000;     // total size for test list.
const int PickSize = 1000;      // the number to be test from back of list.

template <class T>
using MyAllocator = list::allocator<T>;  // replace the std::allocator with my allocator
using Point2D = std::pair<int, int>;
using type_name = char;
using T_Vec = std::list<type_name, MyAllocator<type_name>>;

using hiclock = std::chrono::high_resolution_clock;
using time_point = std::chrono::time_point<hiclock>;
using duration = std::chrono::duration<double>;
using std::chrono::duration_cast;

time_point a_begin, a_end;

int main() {

    std::cout << "------------ Test case for MyAllocator of list ------------" << std::endl;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, TestSize);

    time_point begin, end;

    a_begin = hiclock::now();
    begin = hiclock::now();
    std::list<T_Vec, MyAllocator<T_Vec>> listdous(TestSize);
    end = hiclock::now();

    std::cout
        << "It takes "
        << duration_cast<duration>(end - begin).count()
        << " seconds to create this list"
        << std::endl;

    // push and pop to change lists' size.
    begin = hiclock::now();
    for (int i = 0; i < TestSize; i++) {
        std::list<type_name, MyAllocator<type_name>> lt;
        std::size_t size = dis(gen);
        for (int j = 0; j < size; ++j) 
            lt.push_back(dis(gen));
        for (int j = 0; j < size / 4; ++j) 
            lt.pop_back();        
        for (int j = 0; j < size / 4; ++j) 
            lt.pop_front();       
        for (int j = 0; j < size; ++j) 
            lt.push_back(dis(gen));
        listdous.push_back(lt);
    }
    end = hiclock::now();

    std::cout
        << "It takes "
        << duration_cast<duration>(end - begin).count()
        << " seconds to deal with "
        << TestSize
        << " lists"
        << std::endl;

    // list resize and clear for the last elements of picksize.
    begin = hiclock::now();
    for (int i = 0; i < PickSize; ++i) {
        int n_size = dis(gen);
        listdous.back().resize(n_size);
        listdous.pop_back();
    }
    for (int i = 0; i < PickSize; ++i) {
        listdous.back().clear();
        listdous.pop_back();
    }
    end = hiclock::now();

    std::cout
        << "It takes "
        << duration_cast<duration>(end - begin).count()
        << " seconds to resize "
        << PickSize
        << " picked lists"
        << std::endl;

    // test if we can make correct assignment.
    {
        double val = 2.333;
        listdous.back().back() = val;
        if (listdous.back().back() == val)
            std::cout << "correct assignment in vecdous" << std::endl;
        else
            std::cout << "incorrect assignment in vecdous" << std::endl;
    }
    a_end = hiclock::now();

    std::cout
        << "It takes "
        << duration_cast<duration>(a_end - a_begin).count()
        << " seconds in total"
        << std::endl;
    return 0;
}