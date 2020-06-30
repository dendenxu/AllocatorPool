#pragma once

#include <cassert>
#include <iostream>
#include <limits>
#include <map>
#include <vector>

#include "myAllocator_trait.hpp"
#include "pool.hpp"

namespace vector
{
template <typename T, typename Tr = trait::Allocator_Traits<T> >
class allocator
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

    const size_type chunk_num = 2;

    template <typename U>
    struct rebind {
        typedef allocator<U> other;
    };

    // constructor
    inline allocator() noexcept {};
    inline allocator(const allocator&) noexcept {};
    template <typename U>
    inline allocator(const allocator<U>&) noexcept {};
    template <typename U, typename Trr>
    inline allocator(const allocator<U, Trr>&) noexcept {};

    // destructor
    inline ~allocator(){};

    // address (left before c++17)
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    // allocate (left before c++17)
    pointer allocate(size_type n)
    {
        size_type size = sizeof(value_type) * n;
        if (_mpool == nullptr) {
            _mpool = new mem::MonoMemory(size * chunk_num);
            pointer ptr = static_cast<pointer>(_mpool->get(size));
            return ptr;
        }

        if (_mpool->free_count() > size) {
            pointer ptr = static_cast<pointer>(_mpool->get(size));
            return ptr;
        } else {
            _current_pool = new mem::MonoMemory(size * chunk_num);
            pointer ptr = static_cast<pointer>(_current_pool->get(size));
            return ptr;
        }
    }

    // deallocate (left before c++17)
    void deallocate(pointer p, size_type n)
    {
        // n must be consistent with the allocated space.
        assert(p != nullptr);
        if (_current_pool != nullptr) {
            _mpool->~MonoMemory();
            _mpool = _current_pool;
            _current_pool = nullptr;
        }
    }
    // max_size
    inline size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max() / (sizeof(value_type) + sizeof(void*));
    }
    // construct
    inline void construct(pointer p, const_reference val)
    {
        new (p) value_type(val);
    }

    // destruct
    template <typename U>
    inline void destroy(U* p)
    {
        p->~U();
    }

   private:
    //std::vector<mem::MonoMemory> _mpools;
    mem::MonoMemory* _mpool = nullptr;
    mem::MonoMemory* _current_pool = nullptr;
};

// template< typename T1, typename Tr1, typename T2, typename Tr2 >
// inline bool operator==(const allocator<T1, Tr1>&, const allocator<T2, Tr2>&) { return true; }
// template< typename T, typename Tr>
// inline bool operator==(const allocator<T, Tr>&, const allocator<T, Tr>&) { return true; }
// template< typename T1, typename Tr1, typename T2, typename Tr2 >
// inline bool operator!=(const allocator<T1, Tr1>&, const allocator<T2, Tr2>&) { return false; }
// template< typename T, typename Tr>
// inline bool operator!=(const allocator<T, Tr>&, const allocator<T, Tr>&) { return false; }
}  // namespace vector

namespace list
{
template <typename T, typename Tr = trait::Allocator_Traits<T> >
class allocator
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

    const size_type chunk_size = 1;

    template <typename U>
    struct rebind {
        typedef allocator<U> other;
    };

    // constructor
    inline allocator() noexcept {};
    inline allocator(const allocator&) noexcept {};
    template <typename U>
    inline allocator(const allocator<U>&) noexcept {};
    template <typename U, typename Trr>
    inline allocator(const allocator<U, Trr>&) noexcept {};

    // destructor
    inline ~allocator(){};

    // address (left before c++17)
    inline pointer address(reference r) { return &r; }
    inline const_pointer address(const_reference r) { return &r; }

    // allocate
    pointer allocate(size_type n)
    {
        if (sizeof(pointer) > sizeof(T)) return reinterpret_cast<pointer>(::operator new(n * sizeof(T)));
        if (_mpools.empty() == true) {
            // first allocator memory for user
            mem::PoolMemory* pool = new mem::PoolMemory(sizeof(T), chunk_size);
            _mpools.push_back(pool);
            pointer ptr = static_cast<pointer>(pool->get());
            return ptr;
        } else {
            mem::PoolMemory* pool = _mpools.back();
            if (pool->full() == false) {
                pointer ptr = static_cast<pointer>(pool->get());
                return ptr;
            } else {
                //we need to allocate a new one by expand.
                pool = new mem::PoolMemory(sizeof(T), pool->capacity() * 2);
                _mpools.push_back(pool);
                //return a new free block.
                pointer ptr = static_cast<pointer>(pool->get());
                return ptr;
            }
        }
    }
    // deallocate
    void deallocate(pointer p, size_type n)
    {
        // set p free when it's deallocate.
        // assert(p != nullptr);
        if (sizeof(pointer) > sizeof(T)) ::operator delete(p);
        if (_mpools.empty() == false) {
            mem::PoolMemory* pool = _mpools.back();
            void* fblock = static_cast<void*>(p);
            pool->free(fblock);
        }
    }
    // max_size
    inline size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max() / (sizeof(value_type) + sizeof(void*));
    }
    // construct (left before c++17)
    template <typename... Args>
    inline void construct(pointer p, Args&&... args)
    {
        new (p) value_type(std::forward<Args>(args)...);
    }

    // destruct (left before c++17)
    inline void destroy(pointer p)
    {
        p->~value_type();
    }

   private:
    std::vector<mem::PoolMemory*> _mpools;  // memory resource for management
};

// template< typename T1, typename Tr1, typename T2, typename Tr2 >
// inline bool operator==(const allocator<T1, Tr1>&, const allocator<T2, Tr2>&) { return true; }
// template< typename T, typename Tr>
// inline bool operator==(const allocator<T, Tr>&, const allocator<T, Tr>&) { return true; }
// template< typename T1, typename Tr1, typename T2, typename Tr2 >
// inline bool operator!=(const allocator<T1, Tr1>&, const allocator<T2, Tr2>&) { return false; }
// template< typename T, typename Tr>
// inline bool operator!=(const allocator<T, Tr>&, const allocator<T, Tr>&) { return false; }
}  // namespace list