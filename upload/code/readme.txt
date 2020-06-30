# File Specification

- `main.cpp`: The test file of PTA
- `myAllocator.hpp`: Allocator class definition and declaration
- `myAllocator_trait.hpp`: Allocator Trait class definition and declaration
- `naive.cpp`: Naive Allocator implementation and declaration (using new and delete on every `allocate` and `deallocate` call)
- `pool.hpp`: Declaration of Pool Memory Resource and Monotonic Memory Resource
- `pool.cpp`: Implementation of Pool Memory Resource and Monotonic Memory Resource
- `test.cpp`: Test file for Pool Memory and Monotonic Memory
- `test_list.cpp`: Allocator test file for std::list
- `test_vector.cpp`: Allocator test file for std::vector

# Source With a main

`main.cpp`, `test.cpp`, `test_list.cpp`, `test_vector.cpp`

# Compile

If you're including a header file and you want to use the class defined in it, do remember to add the corresponding `.cpp` file to compilation list.
And you would love to add a `-std=c++2a` standard specifier (c++17 would work too)
For example, to compile `test.cpp`, you should also add `pool.cpp` in the compilation list:
```bash
clang++ -Ofast -std=c++2a test.cpp pool.cpp -o test
```