# `Memory Resource`: 内存资源管理

## `Byte Memory`

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

## `Memory Pool`

### 概要

我们通过内存池这一结构来管理内存的分配与释放，我们提供类似于普通线性堆栈式内存资源的接口，不额外占据内存以实现一个内存链表。

由于我们将链表地址直接映射到了需要分配的内存池中，这一结构的空间复杂度为`O(1)`。