# `Memory Resource`：内存资源管理

## `Monotonic Memory`：堆栈式内存资源

### 概要

我们通过最简单的方式实现了一种基于`index`的堆栈式内存资源。

初始化时，这种资源会将其`index`设置为零，并通过初始化参数确定其总体内存资源的大小。这种内存资源保有一个指向大型内存资源的指针，并通过开放接口来让外界获得其中的内存地址。在对内存进行最大程度利用的同时保持极高的性能。

### 具体实现

我们会在类内保有一个指向大型内存块的资源。并保有一个记录当前已经被分配内存的`index`。在分配内存时调整这一`index`，使其匹配当前分配出去的内存大小。当调用`free`接口进行内存释放时，我们会判断给出的指针与调整`index`之前的指针是否匹配。

我们提供了以下接口：

```c++
MonoMemory(const std::size_t size);
MonoMemory(const std::size_t size, std::byte *pointer);
std::size_t free_count();
std::size_t size();
std::size_t capacity();
bool empty();
bool full();
bool has_upper();
void *get(std::size_t size);
void free(void *pblock, std::size_t size);
void free(std::size_t size);
```

- 其中最核心的接口是`get`与`free`

    1. `get`可以为上层资源提供参数中说明数量的内存，若无法正常分配则会返回`nullptr`。
    2. `free`函数会收回一定数量的内存，其中第一个参数的内存地址在具体实现时可有可无，但可以用于判断用户是否在正确的地方释放了正确的内存，方便调试与检测内存泄漏。

- 在初始化`MonoMemory`时，我们提供了两个初始化版本。

    1. 只确定内存资源大小的初始化：`ByteMemory(const std::size_t size);`会调用系统的`new`和`free`来获得`size` 所需大小的内存资源，对应的，在本`MonoMemory`被释放时，他会调用相应位置的`delete`操作符以清除对应位置的内存。
    2. 确定内存资源大小与初始指针的初始化：`MonoMemory(const std::size_t size, std::byte *pointer);`不会调用系统的相应内存分配操作，而是直接使用已经给出的内存资源。用户必须保证这一资源至少与参数`size`中所示的一样大，否则程序的行为将是未定义的。

- 我们还提供了一些管理用接口

    1. `std::size_t free_count();`返回当前该资源可用内存大小。
    2. `std::size_t size();`返回当前资源已用内存大小。
    3. `std::size_t capacity();`返回当前资源最多的可用内存大小。
    4. `bool empty();`用以判断当前资源是否为空资源（没有内存被分配到上层）。
    5. `bool full();`用以判断当前资源是否已分配完（没有可用资源供继续分配）。

具体实现：

```c++

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
    Size of a MonoMemory is: 32
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

类似于`MonoMemory`，我们提供了如下接口：

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
void *get(std::size_t size);
void *get();
void free(void *pblock, std::size_t size);
void free(void *pblock);
```

- 类似的，核心接口是`get`与`free`两个函数，他们提供了分配和释放内存的功能。

    1.  `get`函数会取得当前`free_list`的最前端内存`m_phead`，并将内部的前端记录器更新为他的下一段内存的指针，也就是我们在初始化时就已经写入对应内存的。
    2. `free`函数会接受一个内存地址，它无法判断这段内存地址究竟是否来自以前分配出去的，它会将参数中的地址作为新的`m_phead`也就是`free_list`的头部，并将旧头部的地址储存到这一个内存下。
    3.  为了匹配接口，我们也提供了传入`size`的版本，我们推荐统一使用这种接口，因为程序可以帮忙检查上层代码渴望的内存数量与我们能够提供的是否相等。当然，我们对此使用了`assert`，这种错误一旦出现就将是致命的。

- 类似的，初始化过程中，我们提供了两种类型，上层代码可以通过选择是否传入`p_memory`参数来选择是否让`PoolMemory`变量管理内存。

    不同于`MonoMemory`，两个构造函数都会调用内部接口`init_memory`以初始化`m_pmemory`指向的内存段（将内存段按顺序填充为下一段内存的头地址，构成一个链表。除了最后一段存储一个空指针（`nullptr`））。

- 其他控制接口类似于我们在`MonoMemory`中使用的。

具体实现

```c++

```

### 特性

由于我们使用内存池的方式管理相应的内存，上层用户代码可以将此结构想像成一个池子：我们可以随意从中取出一些内存空间，然后将以前取出的空间放回（只要我们保证放回的空间就是原来已经取出的），而不需要像`MonoMemory`那样担心存取的顺序问题。用户代码完全可以将所有从中取出的空间一视同仁。

但对应的，有失就有得，`PoolMemory`无法处理任意大小的内存分配请求。这是显而易见的，如果我们考虑到内存池的基本结构。

优点：

1. 支持任意顺序的内存分配和释放。
2. 在初始化时支持任意数量与任意分块大小的内存分配与释放（在实际物理内存充足的情况下）。
3. 不会产生内存泄漏，只有当`m_pmemory`指向的大块内存空间已经被全部耗尽时才会停止继续分配内存。
4. 不会产生内存碎片，纵使在一系列的随机分配和释放过程中，我们会发现内存的顺序被打乱，且为分配内存与已分配内存会交替出现，但我们通过链表结构解决了这一问题。复杂的结构通过链表完整的到完整链接。

缺点：

1. 相对于`MonoMemory`的单变量操作，`PoolMemory`会进行内存和指针操作，缓存命中率相对较低，且取内存往往比普通的加法操作更耗时，因此虽然两者的时间复杂度都为常数级别，但`PoolMemory`的分配和释放速度会相对慢于`MonoMemory`。
2. 这一内存资源无法支持任意大小内存的分配和释放要求，这是由`PoolMemory`的结构决定的。

## `Testing`：测试

### 测试基本流程

1. 根据随机生成器结果决定是否让被测试内存资源手动分配内存。

2. 构建内存资源并记录构建速度，打印本资源相关信息。

3. 连续调用内存资源的`get`接口以获得相应大小的内存。

    1. 对于`MonoMemory`，我们根据现有`free_count`获取一个小于等于此数量的随机大小的内存。并进入循环，直到整个内存资源被耗尽。我们将获取到的内存指针以及其大小按照顺序存入一个向量供日后使用。
    2. 对于`PoolMemory`，我们调用`get`的次数与内存资源的总大小相等，恰好将内存资源耗尽。虽然我们无法从`PoolMemory`中获取任意大小的内存，但我们可以打乱内存分配顺序。我们在获取内存指针后将其随机插入一个指针向量。

4. 再次调用`get`并检查是否出发异常。

5. 连续调用内存资源的`free`接口以清空刚刚获取的内存。

    1. 对于`MonoMemory`，我们按照堆栈式顺序清空先前压入的相应内存资源。虽然资源的大小时随机生成的。
    2. 对于`PoolMemory`，我们从现有的内存指针向量中随机选取一个指针，乱序调用`free`接口使得内存资源中的`free_list`被打乱，并检测这种方式下资源的效率与正确性。

6. 我们累加调用内存资源接口的时间，并通过`chrono`的高精度时钟记录总时间，用以判断内存的读写效率。

7. 最后我们进行随机分配效率测试，我们迭代`actual_size`次（这个数目是通过随机生成器得到的），每次迭代中调用一次01随机生成器，对应两种不同的结果分别进行分配和释放操作。

    类似的，对于`MonoMemory`我们按照堆栈顺序分配和释放这些资源；对于`PoolMemory`我们使用随机顺序。

    类似的，我们仍然记录并累加每次操作的时间之和，并以此为依据获得平均运行时间。

### 测试代码具体实现

```c++

```

### 测试结果

```c++
[INFO] We're doing the allocation manually
It takes 5.896e-06 seconds to create and initialize the byte memory resource
Our actual size is: 1550
Initial size of this byte memory: 0
Initial capacity of this byte memory: 198400
Initial free space of this byte memory: 198400
Size of the byte memory variable: 32
Size of the byte memory: 198400
Is the memory resource full? Yes
It takes 4.77e-07 seconds to get this much times, which averages to 3.66923e-08 seconds per operations
It takes 5.25e-07 seconds to free this much times, which averages to 4.03846e-08 seconds per operations
The pool's current size: 197343
The ptrs's current size: 6
[INFO] We're doing the allocation ahead of time
It takes 1.2e-07 seconds to create and initialize the byte memory resource
Our actual size is: 1944
Initial size of this byte memory: 0
Initial capacity of this byte memory: 248832
Initial free space of this byte memory: 248832
Size of the byte memory variable: 32
Size of the byte memory: 248832
Is the memory resource full? Yes
It takes 5.97e-07 seconds to get this much times, which averages to 3.51176e-08 seconds per operations
It takes 6.63e-07 seconds to free this much times, which averages to 3.9e-08 seconds per operations
The pool's current size: 230980
The ptrs's current size: 2
[INFO] We're doing the allocation ahead of time
It takes 7.3e-08 seconds to create and initialize the byte memory resource
Our actual size is: 1695
Initial size of this byte memory: 0
Initial capacity of this byte memory: 216960
Initial free space of this byte memory: 216960
Size of the byte memory variable: 32
Size of the byte memory: 216960
Is the memory resource full? Yes
It takes 5.69e-07 seconds to get this much times, which averages to 3.55625e-08 seconds per operations
It takes 6.14e-07 seconds to free this much times, which averages to 3.8375e-08 seconds per operations
The pool's current size: 216958
The ptrs's current size: 11
[INFO] We're doing the allocation manually
It takes 1.991e-06 seconds to create and initialize the byte memory resource
Our actual size is: 3050
Initial size of this byte memory: 0
Initial capacity of this byte memory: 390400
Initial free space of this byte memory: 390400
Size of the byte memory variable: 32
Size of the byte memory: 390400
Is the memory resource full? Yes
It takes 8.43e-07 seconds to get this much times, which averages to 3.5125e-08 seconds per operations
It takes 9.38e-07 seconds to free this much times, which averages to 3.90833e-08 seconds per operations
The pool's current size: 390056
The ptrs's current size: 6
[INFO] We're doing the allocation manually
It takes 2.22e-07 seconds to create and initialize the byte memory resource
Our actual size is: 1795
Initial size of this byte memory: 0
Initial capacity of this byte memory: 229760
Initial free space of this byte memory: 229760
Size of the byte memory variable: 32
Size of the byte memory: 229760
Is the memory resource full? Yes
It takes 3.25e-07 seconds to get this much times, which averages to 3.61111e-08 seconds per operations
It takes 3.47e-07 seconds to free this much times, which averages to 3.85556e-08 seconds per operations
The pool's current size: 200820
The ptrs's current size: 3
[INFO] We're doing the allocation manually
It takes 0.000121443 seconds to create and initialize the memory pool
Our actual size is: 2953
Initial size of this memory pool: 0
Initial capacity of this memory pool: 2953
Initial free space of this memory pool: 2953
Size of the memory pool variable: 56
Size of the memory pool: 377984
It takes 0.000118293 seconds to get this much times, which averages to 4.00586e-08 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 0.00011813 seconds to free this much times, which averages to 4.00034e-08 seconds per operations
The pool's current size: 49
The ptrs's current size: 49
Is the memory pool eventually empty? Yes
[INFO] We're doing the allocation ahead of time
It takes 1.8677e-05 seconds to create and initialize the memory pool
Our actual size is: 1667
Initial size of this memory pool: 0
Initial capacity of this memory pool: 1667
Initial free space of this memory pool: 1667
Size of the memory pool variable: 56
Size of the memory pool: 213376
It takes 6.386e-05 seconds to get this much times, which averages to 3.83083e-08 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 6.7134e-05 seconds to free this much times, which averages to 4.02723e-08 seconds per operations
The pool's current size: 23
The ptrs's current size: 23
Is the memory pool eventually empty? Yes
[INFO] We're doing the allocation ahead of time
It takes 0.000129775 seconds to create and initialize the memory pool
Our actual size is: 1444
Initial size of this memory pool: 0
Initial capacity of this memory pool: 1444
Initial free space of this memory pool: 1444
Size of the memory pool variable: 56
Size of the memory pool: 184832
It takes 8.6652e-05 seconds to get this much times, which averages to 6.00083e-08 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 9.5426e-05 seconds to free this much times, which averages to 6.60845e-08 seconds per operations
The pool's current size: 12
The ptrs's current size: 12
Is the memory pool eventually empty? Yes
[INFO] We're doing the allocation manually
It takes 3.1879e-05 seconds to create and initialize the memory pool
Our actual size is: 2391
Initial size of this memory pool: 0
Initial capacity of this memory pool: 2391
Initial free space of this memory pool: 2391
Size of the memory pool variable: 56
Size of the memory pool: 306048
It takes 0.000239563 seconds to get this much times, which averages to 1.00194e-07 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 0.000117256 seconds to free this much times, which averages to 4.90406e-08 seconds per operations
The pool's current size: 19
The ptrs's current size: 19
Is the memory pool eventually empty? Yes
[INFO] We're doing the allocation ahead of time
It takes 1.413e-05 seconds to create and initialize the memory pool
Our actual size is: 2191
Initial size of this memory pool: 0
Initial capacity of this memory pool: 2191
Initial free space of this memory pool: 2191
Size of the memory pool variable: 56
Size of the memory pool: 280448
It takes 8.2953e-05 seconds to get this much times, which averages to 3.78608e-08 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 8.679e-05 seconds to free this much times, which averages to 3.9612e-08 seconds per operations
The pool's current size: 57
The ptrs's current size: 57
Is the memory pool eventually empty? Yes
Size of a type is: 128
Size of a bitset<128> is: 16
Size of a std::size_t is: 8
Size of a void * is: 8
Size of a bool is: 1
Size of a PoolMemory is: 56
Size of a MonoMemory is: 32
Test is completed, bye.
```

大测试结果：

```c++
[INFO] We're doing the allocation manually
It takes 5.912e-06 seconds to create and initialize the byte memory resource
Our actual size is: 148127
Initial size of this byte memory: 0
Initial capacity of this byte memory: 18960256
Initial free space of this byte memory: 18960256
Size of the byte memory variable: 32
Size of the byte memory: 18960256
Is the memory resource full? Yes
It takes 7.64e-07 seconds to get this much times, which averages to 3.47273e-08 seconds per operations
It takes 8.2e-07 seconds to free this much times, which averages to 3.72727e-08 seconds per operations
The pool's current size: 18859257
The ptrs's current size: 3
[INFO] We're doing the allocation manually
It takes 9.642e-06 seconds to create and initialize the byte memory resource
Our actual size is: 89108
Initial size of this byte memory: 0
Initial capacity of this byte memory: 11405824
Initial free space of this byte memory: 11405824
Size of the byte memory variable: 32
Size of the byte memory: 11405824
Is the memory resource full? Yes
It takes 4.7e-07 seconds to get this much times, which averages to 3.35714e-08 seconds per operations
It takes 5.41e-07 seconds to free this much times, which averages to 3.86429e-08 seconds per operations
The pool's current size: 11380671
The ptrs's current size: 4
[INFO] We're doing the allocation ahead of time
It takes 1.0632e-05 seconds to create and initialize the byte memory resource
Our actual size is: 145303
Initial size of this byte memory: 0
Initial capacity of this byte memory: 18598784
Initial free space of this byte memory: 18598784
Size of the byte memory variable: 32
Size of the byte memory: 18598784
Is the memory resource full? Yes
It takes 5.95e-07 seconds to get this much times, which averages to 3.5e-08 seconds per operations
It takes 6.56e-07 seconds to free this much times, which averages to 3.85882e-08 seconds per operations
The pool's current size: 18598647
The ptrs's current size: 11
[INFO] We're doing the allocation ahead of time
It takes 9.128e-06 seconds to create and initialize the byte memory resource
Our actual size is: 188890
Initial size of this byte memory: 0
Initial capacity of this byte memory: 24177920
Initial free space of this byte memory: 24177920
Size of the byte memory variable: 32
Size of the byte memory: 24177920
Is the memory resource full? Yes
It takes 4.26e-07 seconds to get this much times, which averages to 3.55e-08 seconds per operations
It takes 4.77e-07 seconds to free this much times, which averages to 3.975e-08 seconds per operations
The pool's current size: 24177878
The ptrs's current size: 10
[INFO] We're doing the allocation manually
It takes 6.35e-07 seconds to create and initialize the byte memory resource
Our actual size is: 83817
Initial size of this byte memory: 0
Initial capacity of this byte memory: 10728576
Initial free space of this byte memory: 10728576
Size of the byte memory variable: 32
Size of the byte memory: 10728576
Is the memory resource full? Yes
It takes 6.18e-07 seconds to get this much times, which averages to 3.63529e-08 seconds per operations
It takes 6.67e-07 seconds to free this much times, which averages to 3.92353e-08 seconds per operations
The pool's current size: 9098302
The ptrs's current size: 3
[INFO] We're doing the allocation manually
It takes 0.00730747 seconds to create and initialize the memory pool
Our actual size is: 182218
Initial size of this memory pool: 0
Initial capacity of this memory pool: 182218
Initial free space of this memory pool: 182218
Size of the memory pool variable: 56
Size of the memory pool: 23323904
It takes 0.0163786 seconds to get this much times, which averages to 8.98847e-08 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 0.010426 seconds to free this much times, which averages to 5.72174e-08 seconds per operations
The pool's current size: 262
The ptrs's current size: 262
Is the memory pool eventually empty? Yes
[INFO] We're doing the allocation ahead of time
It takes 0.00335973 seconds to create and initialize the memory pool
Our actual size is: 84421
Initial size of this memory pool: 0
Initial capacity of this memory pool: 84421
Initial free space of this memory pool: 84421
Size of the memory pool variable: 56
Size of the memory pool: 10805888
It takes 0.00596156 seconds to get this much times, which averages to 7.0617e-08 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 0.0048064 seconds to free this much times, which averages to 5.69337e-08 seconds per operations
The pool's current size: 5
The ptrs's current size: 5
Is the memory pool eventually empty? Yes
[INFO] We're doing the allocation ahead of time
It takes 0.00368722 seconds to create and initialize the memory pool
Our actual size is: 143090
Initial size of this memory pool: 0
Initial capacity of this memory pool: 143090
Initial free space of this memory pool: 143090
Size of the memory pool variable: 56
Size of the memory pool: 18315520
It takes 0.0124636 seconds to get this much times, which averages to 8.71029e-08 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 0.00801772 seconds to free this much times, which averages to 5.60327e-08 seconds per operations
The pool's current size: 20
The ptrs's current size: 20
Is the memory pool eventually empty? Yes
[INFO] We're doing the allocation manually
It takes 0.00783581 seconds to create and initialize the memory pool
Our actual size is: 193340
Initial size of this memory pool: 0
Initial capacity of this memory pool: 193340
Initial free space of this memory pool: 193340
Size of the memory pool variable: 56
Size of the memory pool: 24747520
It takes 0.0173287 seconds to get this much times, which averages to 8.96279e-08 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 0.0115603 seconds to free this much times, which averages to 5.97926e-08 seconds per operations
The pool's current size: 106
The ptrs's current size: 106
Is the memory pool eventually empty? Yes
[INFO] We're doing the allocation manually
It takes 0.00285732 seconds to create and initialize the memory pool
Our actual size is: 153750
Initial size of this memory pool: 0
Initial capacity of this memory pool: 153750
Initial free space of this memory pool: 153750
Size of the memory pool variable: 56
Size of the memory pool: 19680000
It takes 0.0130964 seconds to get this much times, which averages to 8.51799e-08 seconds per operations
ERROR get: out of memory blocks
std::bad_alloc
It takes 0.00857946 seconds to free this much times, which averages to 5.58014e-08 seconds per operations
The pool's current size: 16
The ptrs's current size: 16
Is the memory pool eventually empty? Yes
Size of a type is: 128
Size of a bitset<128> is: 16
Size of a std::size_t is: 8
Size of a void * is: 8
Size of a bool is: 1
Size of a PoolMemory is: 56
Size of a MonoMemory is: 32
Test is completed, bye.
```

