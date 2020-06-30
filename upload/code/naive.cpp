#include <iostream>
#include <limits>
#include <memory>
#include <vector>
#include <list>
#include <random>
#include <ctime>

namespace oop {
    template <typename T>
    class Allocator
    {
    public:
        using value_type = T;
        using pointer = T*;
        using reference = T&;
        using const_pointer = const T*;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using propagate_on_container_move_assignment = std::true_type;
        using is_always_equal = std::true_type;


        template <typename U> struct rebind {
            typedef Allocator<U> other;
        };

        // constructor
        Allocator() noexcept {}
        Allocator(const Allocator&) noexcept {}
        template <typename U>
        Allocator(const Allocator<U>&) noexcept {}

        // destructor
        ~Allocator() {};

        // address(deprecated in C++17)
        pointer address(reference x) const noexcept { return &x; }
        const_pointer address(const_reference x) const noexcept { return &x; }

        // allocate
        pointer allocate(size_type n) {
            return reinterpret_cast<pointer>(::operator new(n * sizeof(T)));
        }
        // deallocate
        void deallocate(pointer p, size_type n) {
            ::operator delete(p);
        }

        // max_size
        size_type max_size() const noexcept {
            return std::numeric_limits<size_type>::max() / sizeof(value_type);
        }
    private:

    };
    template< typename T1, typename T2 >
    inline bool operator==(const Allocator<T1>&, const Allocator<T2>&) { return true; }
    template< typename T>
    inline bool operator==(const Allocator<T>&, const Allocator<T>&) { return true; }
    template< typename T1, typename T2 >
    inline bool operator!=(const Allocator<T1>&, const Allocator<T2>&) { return false; }
    template< typename T>
    inline bool operator!=(const Allocator<T>&, const Allocator<T>&) { return false; }
} // namespace oop

//const int TestSize = 10;
//const int PickSize = 1;
//
//template <class T>
//using MyAllocator = oop::Allocator<T>;  // replace the std::allocator with your allocator
//using Point2D = std::pair<int, int>;
//using type_name = double;
//using T_Vec = std::vector<type_name, MyAllocator<type_name>>;
//
//int main() {
//
//    std::random_device rd;
//    std::mt19937 gen(rd());
//    std::uniform_int_distribution<> dis(1, TestSize);
//
//    std::vector< type_name, MyAllocator<type_name> > l0, l1;
//    std::vector<T_Vec, MyAllocator<T_Vec>> vecints(TestSize);
//
//    for (int i = 0; i < TestSize; i++) {
//        vecints[i].resize(dis(gen));
//    }
//
//    //// vector resize randomly
//    //for (int i = 0; i < PickSize; i++) {
//    //    int idx = dis(gen) - 1;
//    //    int size = dis(gen);
//    //    vecints[idx].resize(size);
//    //}
//
//    for (int i = 0; i < vecints.size(); ++i) {
//        std::cout << vecints[i].size() << std::endl;
//    }
//    {
//        int val = 10;
//        int idx1 = dis(gen) - 1;
//        int idx2 = vecints[idx1].size() / 2;
//        vecints[idx1][idx2] = val;
//        if (vecints[idx1][idx2] == val)
//            std::cout << "correct assignment in vecints: " << idx1 << std::endl;
//        else
//            std::cout << "incorrect assignment in vecints: " << idx1 << std::endl;
//    }
//
//    //l0.resize(8);
//    return 0;
//}