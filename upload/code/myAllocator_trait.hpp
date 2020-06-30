#pragma once

namespace trait
{
template <typename T>
class Allocator_Traits
{
   public:
    template <typename U>
    struct rebind {
        typedef Allocator_Traits<U> other;
    };

    explicit Allocator_Traits() {}
    explicit Allocator_Traits(Allocator_Traits const&) {}
    template <typename U>
    explicit Allocator_Traits(Allocator_Traits<U> const&)
    {
    }

    ~Allocator_Traits() {}

    // address
    inline T* address(T& r) { return &r; }
    inline T const* address(T const& r) { return &r; }

    // construct(deprecated in C++17)
    template <typename U, typename... Args>
    void construct(U* p, Args&&... args)
    {
        new (p) U(std::forward<Args>(args)...);
    }

    // destruct
    template <typename U>
    void destroy(U* p)
    {
        p->~U();
    }
};
}  // namespace trait