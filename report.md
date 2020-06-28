# `Memory Resource`：内存资源管理

## `Byte Memory`：堆栈式内存资源

### 概要

我们通过最简单的方式实现了一种基于`index`的堆栈式内存资源。

初始化时，这种资源会将其`index`设置为零，并通过初始化参数确定其总体内存资源的大小。这种内存资源保有一个指向大型内存资源的指针，并通过开放接口来让外界获得其中的内存地址。在对内存进行最大程度利用的同时保持极高的性能。

### 具体实现

我们会在类内保有一个指向大型内存块的资源。并保有一个记录当前已经被分配内存的`index`。在分配内存时调整这一`index`，使其匹配当前分配出去的内存大小。当调用`free`接口进行内存释放时，我们会判断给出的指针与调整`index`之前的指针是否匹配。

我们提供了以下接口：

```c++
ByteMemory(const std::size_t size);
ByteMemory(const std::size_t size, std::byte *pointer);
std::size_t free_count();
std::size_t size();
std::size_t capacity();
bool empty();
bool full();
void *get(std::size_t size);
void free(void *pblock, std::size_t size);
```

- 其中最核心的接口是`get`与`free`

    1. `get`可以为上层资源提供参数中说明数量的内存，若无法正常分配则会返回`nullptr`。
    2. `free`函数会收回一定数量的内存，其中第一个参数的内存地址在具体实现时可有可无，但可以用于判断用户是否在正确的地方释放了正确的内存，方便调试与检测内存泄漏。

- 在初始化`ByteMemory`时，我们提供了两个初始化版本。

    1. 只确定内存资源大小的初始化：`ByteMemory(const std::size_t size);`会调用系统的`new`和`free`来获得`size` 所需大小的内存资源，对应的，在本`ByteMemory`被释放时，他会调用相应位置的`delete`操作符以清除对应位置的内存。
    2. 确定内存资源大小与初始指针的初始化：`ByteMemory(const std::size_t size, std::byte *pointer);`不会调用系统的相应内存分配操作，而是直接使用已经给出的内存资源。用户必须保证这一资源至少与参数`size`中所示的一样大，否则程序的行为将是未定义的。

- 我们还提供了一些管理用接口

    1. `std::size_t free_count();`返回当前该资源可用内存大小。
    2. `std::size_t size();`返回当前资源已用内存大小。
    3. `std::size_t capacity();`返回当前资源最多的可用内存大小。
    4. `bool empty();`用以判断当前资源是否为空资源（没有内存被分配到上层）。
    5. `bool full();`用以判断当前资源是否已分配完（没有可用资源供继续分配）。

具体实现：

```c++
namespace mem
{
class ByteMemory
{
   public:
    ByteMemory(const std::size_t size) : m_total_size(size), m_index(0), m_is_manual(true) { m_pmemory = new std::byte[size]; }
    ByteMemory(const std::size_t size, std::byte *pointer) : m_pmemory(pointer), m_index(0), m_total_size(size), m_is_manual(false) {}

    ByteMemory(const ByteMemory &alloc) = delete;           // delete copy constructor
    ByteMemory &operator=(const ByteMemory &rhs) = delete;  // delete copy-assignment operator
    ByteMemory(ByteMemory &&alloc) = delete;                // delete move constructor
    ByteMemory &operator=(ByteMemory &&rhs) = delete;       // delete move-assignment operator

    ~ByteMemory()
    {
        if (m_is_manual) {
            delete[] m_pmemory;
        }
    }  // delete the pre-allocated byte chunk chunk

    std::size_t free_count() { return m_total_size - m_index; }  // return number of free blocks inside the byte chunk
    std::size_t size() { return m_index; }                       // return the number of used space in the byte chunk
    std::size_t capacity() { return m_total_size; }              // return total number of blocks that this pool can hold
    bool empty() { return m_index == 0; }                        // return whether the byte chunk is empty
    bool full() { return m_index == m_total_size; }              // return whether the byte chunk is full
    bool has_upper() { return ~m_is_manual; }                              // return whether m_pmemory's raw mem comes from an upper stream

    // return a nullptr if the byte chunk is already full
    // else this returns a pointer to an block whose size(still raw memory) is m_block_sz_bytes
    void *get(std::size_t size)
    {
        if (m_index + size > m_total_size) {
            std::cerr << "[ERROR] Unable to handle the allocation, too large for this chunk." << std::endl;
            return nullptr;
        } else {
            void *ptr = m_pmemory + m_index;
            m_index += size;
            return ptr;
        }
    }

    // make sure the pblock is one of the pointers that you get from this byte chunk
    void free(void *pblock, std::size_t size)
    {
        if (m_index < size) {  // this should not happen if you're calling it right
            std::cerr << "[ERROR] You can only give back what you've taken away." << std::endl;
        } else {
            m_index -= size;
            if (m_index + m_pmemory != pblock) {  // this should not happen if you're calling it right
                std::cerr << "[ERROR] You can only give back what you've taken away, and in the right order" << std::endl;
            }
        }
    }

   private:
    std::byte *m_pmemory;      // pointer to the byte array
    std::size_t m_index;       // current index of the byte array
    std::size_t m_total_size;  // total number of blocks
    bool m_is_manual;          // whether the m_pmemory is manually allocated by us
};

}  // namespace mem
```

### 特性

基于这一单向增长，反向减少的特性，这种内存资源仅支持类似堆栈的分配机制：

- **先进后出**，只有当堆栈顶端的内存被释放后，其下部资源才有可能被合法释放。

当然，由于内存资源是线性管理的，我们也可以利用这一特性来达到更好的大块内存分配效率：

- 当我们连续分配了许多段小内存，我们可以通过调用一次`free`函数来将他们全部释放。

优点：

1. 这种内存资源的分配和释放速度极快，这两个操作仅仅涉及到对`index`的增加或减少。

     这两个操作的时间复杂度是线性的，无论我们需要的内存大小是多少。

2. 这种内存资源支持变大小的内存分配，这一点可以通过传入参数`std::size_t size`实现。

3. 若严格遵守此资源的分配和释放要求（堆栈形的先进先出），则没有内存泄漏会发生，因为所有分配和释放操作都是连续的。

4. 同上一点，该资源不会在内部产生内存碎片，因为分配和释放都是完全连续的。

5. 由于我们支持在构建该内存资源时传入指针，此内存资源是灵活的，可以使用其他资源提供的指针。

缺点：

1. 内存资源需严格按照堆栈要求进行分配释放才符合要求，这严重限制了此类内存资源的应用面。

2. 成员变量的大小为`32 Bytes`，这意味着如果我们通过这一个类管理的内存与此数量级相当，我们会遇到不小的内存资源浪费。

    实际大小应为：`25 Bytes`，但由于我的机器为8字节对齐，最终大小成为了`32 Bytes`

    ```c++
    Size of a std::size_t is: 8
    Size of a void * is: 8
    Size of a bool is: 1
    Size of a ByteMemory is: 32
    ```

## `Pool Memory`：内存池资源

### 概要

我们通过内存池这一结构来管理内存的分配与释放，我们提供类似于普通线性堆栈式内存资源的接口，不额外占据内存以实现一个内存链表。

由于我们将链表地址直接映射到了需要分配的内存池中，这一结构的空间复杂度为`O(1)`。

但由于我们需要初始化相应位置的内存，创建这一结构的时间复杂度为`O(num_blocks)`。

幸运的是，在内存池资源创建完毕后，获取和释放内存的操作的时间复杂度就变成了`O(1)`。只涉及到几个简单的指针操作与`static_cast<>`。

值得注意的是，由于我们在内存池的原始内存位置直接存放了相应的指针，我们要求池中每一个块的大小至少为`sizeof(void *)`，以实现对内存的更高效利用。

- 若我们不进行这样的要求，让我们做出如下假设：

    1. 指针大小（`sizeof(void *)`）为：`8 Bytes`
    2. 内存池块大小为`4 Bytes`

    容易发现，在更新某一块内存中储存的指针时，它会与其相邻块的内容发生重叠，造成紊乱。因此上述要求是必须的

- 退一步讲，若我们不进行这样的实现，而是通过一个外部结构（数组或链表）来管理相关内存指针，则会发生严重的内存浪费。

    即使我们的外部资源除了一个8字节的指针外不存储任何信息，这一内存消耗也是严重的：

    外部数据需要的内存量甚至大于实际分配出的内存。

### 具体实现

如上所述，我们通过在原生内存上构建指针来实现这一内存池结构。

类似于`ByteMemory`，我们提供了如下接口：

```c++
PoolMemory(const std::size_t block_sz_bytes, const std::size_t num_blocks);
PoolMemory(const std::size_t block_sz_bytes, const std::size_t num_blocks, std::byte *pmemory);
std::size_t block_size();
std::size_t pool_size();
std::size_t free_count();
std::size_t size();
std::size_t capacity();
bool empty();
bool full();
bool has_upper();
void *get();
void free(void *pblock);
```

- 类似的，核心接口是`get`与`free`两个函数，他们提供了分配和释放内存的功能。

    1.  `get`函数会取得当前`free_list`的最前端内存`m_phead`，并将内部的前端记录器更新为他的下一段内存的指针，也就是我们在初始化时就已经写入对应内存的。
    2. `free`函数会接受一个内存地址，它无法判断这段内存地址究竟是否来自以前分配出去的，它会将参数中的地址作为新的`m_phead`也就是`free_list`的头部，并将旧头部的地址储存到这一个内存下。

- 类似的，初始化过程中，我们提供了两种类型，上层代码可以通过选择是否传入`p_memory`参数来选择是否让`PoolMemory`变量管理内存。

    不同于`ByteMemory`，两个构造函数都会调用内部接口`init_memory`以初始化`m_pmemory`指向的内存段（将内存段按顺序填充为下一段内存的头地址，构成一个链表。除了最后一段存储一个空指针（`nullptr`））。

- 其他控制接口类似于我们在`ByteMemory`中使用的。

具体实现

头文件：

```c++
namespace mem
{
class PoolMemory
{
   public:
    PoolMemory(const std::size_t block_sz_bytes, const std::size_t num_blocks);
    PoolMemory(const std::size_t block_sz_bytes, const std::size_t num_blocks, std::byte *pmemory);

    PoolMemory(const PoolMemory &alloc) = delete;           // delete copy constructor
    PoolMemory &operator=(const PoolMemory &rhs) = delete;  // delete copy-assignment operator
    PoolMemory(PoolMemory &&alloc) = delete;                // delete move constructor
    PoolMemory &operator=(PoolMemory &&rhs) = delete;       // delete move-assignment operator

    ~PoolMemory()
    {
        if (m_is_manual) {
            delete[] m_pmemory;
        }
    }  // delete the pre-allocated memory pool chunk

    std::size_t block_size() { return m_block_sz_bytes; }                  // return block size in byte
    std::size_t pool_size() { return m_pool_sz_bytes; }                    // return memory pool size in byte
    std::size_t free_count() { return m_free_num_blocks; }                 // return number of free blocks inside the memory pool
    std::size_t size() { return m_total_num_blocks - m_free_num_blocks; }  // return the number of used space in the memory pool
    std::size_t capacity() { return m_total_num_blocks; }                  // return total number of blocks that this pool can hold
    bool empty() { return m_free_num_blocks == m_total_num_blocks; }       // return whether the memory pool is empty
    bool full() { return m_free_num_blocks == 0; }                         // return whether the memory pool is full
    bool has_upper() { return ~m_is_manual; }                              // return whether m_pmemory's raw mem comes from an upper stream

    // return a nullptr if the memory pool is already full
    // else this returns a pointer to an block whose size(still raw memory) is m_block_sz_bytes
    void *get();

    // make sure the pblock is one of the pointers that you get from this memory pool
    void free(void *pblock);

   private:
    void init_memory();  // this function will fill the memory with pointers to the next trunk for initialization

    /** Current size of a memory pool variable should be 48 bytes
     *  considering 8 byte for one pointer and size_t on my machine
     */
    std::byte *m_pmemory;            // pointer to the first address of the pool, used to relase all the memory
    void **m_phead;                  // pointer to pointer, used to point to the head of the free list
    std::size_t m_pool_sz_bytes;     //the size in bytes of the pool
    std::size_t m_block_sz_bytes;    // size in bytes of each block
    std::size_t m_free_num_blocks;   // number of blocks
    std::size_t m_total_num_blocks;  // total number of blocks
    bool m_is_manual;                // whether the m_pmemory is manually allocated by us
};

}  // namespace mem
```

源码文件：

```c++
#include "pool.hpp"
using namespace mem;

PoolMemory::PoolMemory(const std::size_t block_sz_bytes, const std::size_t num_blocks)
    : m_pool_sz_bytes(num_blocks * block_sz_bytes),
      m_block_sz_bytes(block_sz_bytes),
      m_free_num_blocks(num_blocks),
      m_total_num_blocks(num_blocks),
      m_is_manual(true)
{
    m_pmemory = new std::byte[m_pool_sz_bytes];  // using byte as memory pool base type
    init_memory();
}

PoolMemory::PoolMemory(const std::size_t block_sz_bytes, const std::size_t num_blocks, std::byte *pmemory)
    : m_pool_sz_bytes(num_blocks * block_sz_bytes),
      m_block_sz_bytes(block_sz_bytes),
      m_free_num_blocks(num_blocks),
      m_total_num_blocks(num_blocks),
      m_is_manual(false),
      m_pmemory(pmemory)  // this memory may have come from a different memory resource
{
    init_memory();
}

void PoolMemory::init_memory()
{
    /** We would want the size of the of the block to be bigger than a pointer */
    assert(sizeof(void *) <= m_block_sz_bytes);
    m_phead = reinterpret_cast<void **>(m_pmemory);  // treat list pointer as a pointer to pointer

    /**
     * We're using uintptr_t to perform arithmetic operations with confidence
     * We're not using void * since, well, it's forbidden to perform arithmetic operations on a void *
     */
    std::uintptr_t start_addr = reinterpret_cast<std::uintptr_t>(m_pmemory);  // where the whole chunk memory begins
    std::uintptr_t end_addr = start_addr + m_pool_sz_bytes;                   // where the chunk memory ends

    /** We use the same space as the actual block to be stored here to store the free list pointers */
    // construct the linked list from raw memory
    for (auto i = 0; i < m_total_num_blocks; i++) {
        std::uintptr_t curr_addr = start_addr + i * m_block_sz_bytes;  // current block's address
        std::uintptr_t next_addr = curr_addr + m_block_sz_bytes;       // next block's address
        void **curr_mem = reinterpret_cast<void **>(curr_addr);        // a pointer, same value as curr_addr to support modification
        if (next_addr >= end_addr) {
            *curr_mem = nullptr;
        } else {
            *curr_mem = reinterpret_cast<void *>(next_addr);
        }
    }
}

void *PoolMemory::get()
{
    // if (m_pmemory == nullptr) {  // This should not happen
    //     std::cerr << "ERROR " << __FUNCTION__ << ": No memory was allocated to this pool" << std::endl;
    //     return nullptr;
    // }

    if (m_phead != nullptr) {
        m_free_num_blocks--;  // decrement the number of free blocks

        void *pblock = static_cast<void *>(m_phead);  // get current free list value
        m_phead = static_cast<void **>(*m_phead);     // update free list head

        return pblock;
    } else {  // out of memory blocks (for an block with size m_block_sz_bytes)
        std::cerr << "ERROR " << __FUNCTION__ << ": out of memory blocks" << std::endl;
        return nullptr;  // if you get a nullptr from a memory pool, it's time to allocate a new one
    }
}

void PoolMemory::free(void *pblock)
{
    if (pblock == nullptr) {
        // do nothing if we're freeing a nullptr
        // although this situation is declared undefined in C++ Standard
        return;
    }

    // if (m_pmemory == nullptr) {  // this should not happen
    //     std::cerr << "ERROR " << __FUNCTION__ << ": No memory was allocated to this pool" << std::endl;
    //     return;
    // }

    m_free_num_blocks++;  // increment the number of blocks

    if (m_phead == nullptr) {  // the free list is full (we can also check this by validating size)
        m_phead = static_cast<void **>(pblock);
        *m_phead = nullptr;
    } else {
        void *ppreturned_block = static_cast<void *>(m_phead);  // temporaryly store the current head as nex block
        m_phead = static_cast<void **>(pblock);
        *m_phead = ppreturned_block;
    }
}
```

### 特性

由于我们使用内存池的方式管理相应的内存，上层用户代码可以将此结构想像成一个池子：我们可以随意从中取出一些内存空间，然后将以前取出的空间放回（只要我们保证放回的空间就是原来已经取出的），而不需要像`ByteMemory`那样担心存取的顺序问题。用户代码完全可以将所有从中取出的空间一视同仁。

但对应的，有失就有得，`PoolMemory`无法处理任意大小的内存分配请求。这是显而易见的，如果我们考虑到内存池的基本结构。

优点：

1. 支持任意顺序的内存分配和释放。
2. 在初始化时支持任意数量与任意分块大小的内存分配与释放（在实际物理内存充足的情况下）。
3. 不会产生内存泄漏，只有当`m_pmemory`指向的大块内存空间已经被全部耗尽时才会停止继续分配内存。
4. 不会产生内存碎片，纵使在一系列的随机分配和释放过程中，我们会发现内存的顺序被打乱，且为分配内存与已分配内存会交替出现，但我们通过链表结构解决了这一问题。复杂的结构通过链表完整的到完整链接。

缺点：

1. 相对于`ByteMemory`的单变量操作，`PoolMemory`会进行内存和指针操作，缓存命中率相对较低，且取内存往往比普通的加法操作更耗时，因此虽然两者的时间复杂度都为常数级别，但`PoolMemory`的分配和释放速度会相对慢于`ByteMemory`。
2. 这一内存资源无法支持任意大小内存的分配和释放要求，这是由`PoolMemory`的结构决定的。

## 测试

