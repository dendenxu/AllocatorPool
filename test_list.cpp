#include "myAllocator.hpp"
#include <iostream>
#include <memory>
#include <list>
#include <random>
#include <chrono>
#include <ratio>

const int TestSize = 10000;
const int PickSize = 1000;

template <class T>
using MyAllocator = std::allocator<T>;  // replace the std::allocator with your allocator
using Point2D = std::pair<int, int>;
using type_name = double;
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


    for (int i = 0; i < PickSize; ++i) {
        int n_size = dis(gen);
        listdous.back().resize(n_size);
        listdous.pop_back();
    }

    std::cout
        << "It takes "
        << duration_cast<duration>(end - begin).count()
        << " seconds to resize "
        << PickSize
        << " picked lists"
        << std::endl;

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