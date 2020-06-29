# Allocator：内存管理接口

## allocator

**allocator**是C++标准库中组件之一，可以实现动态空间的配置、管理、释放，同时为标准库中容器(vector, list等) 提供空间管理。同时也为用户使用提供接口。相比通过new、delete等方式获得或释放内存，allocator也为用户对于内存的管理提供了更灵活的方法。使内存的分配/释放和对象的构造/析构分离开。

allocator提供的接口如下：

```	c++
template <typename T>
class Allocator
{
public:
    // 构造函数
    Allocator();
    Allocator(const Allocator&);
    template <typename U>
    Allocator(const Allocator<U>&);
    // 析构函数
    ~Allocator();
    // 传入x，返回x的地址（c++17后不推荐使用，此处为c++17前保留）
    pointer address(reference x);
    const_pointer address(const_reference x);
    // 分配空间。返回指向该片空间的指针
    pointer allocate(size_type n);
    // 释放空间（p指向的空间）
    void deallocate(pointer p, size_type n);
    // 返回可分配的最大空间（c++17后不推荐使用，此处为c++17前保留）
    size_type max_size();
    // 在指向空间构建对象（c++17后不推荐使用，此处为c++17前保留）
    void construct(pointer p, const_reference val);
    // 在指向空间解构对象（c++17后不推荐使用，此处为c++17前保留）
    void destroy(U* p);
}; 
```

其中，**allocate**和**deallocate**实现allocator的核心功能，标准库通过调用**new**和**delete**实现。频繁的在堆上分配和释放内存，不仅会导致性能的部分损失，还会引入内存碎片。因此，我们可以实现一个适应自己功能的allocator，通过合理的内存管理方式提高内存的使用性能和利用效率。本次大作业，我们为标准库容器**vector**和**list**实现配置器，并通过内存池这一结构管理内存资源。同时，根据实际情况，我们实现了堆栈式内存结构，以更高效地对内存进行利用。

## 内存管理模式设计

### list

#### 接口分析

**list**的空间分配相对较为灵活，其实际可支配的内存空间的释放与分配，都是以一个单位进行的。下面给出一些常用的，可改变list的**size**的方法接口。

* **push_back**：size的值增加一，通过调用一次allocate获得新元素的空间。由于为链表结构，内存地址与原list可以不连续。
* **push_front**：原理与push_back相同。由于为链表结构，内存地址与原list可以不连续。
* **pop_back**：size的值减去一，通过调用一次deallocate释放空间。
* **pop_front**：原理与pop_back相同。
* **insert**：原理与push_back相似。

* **resize**：resize改变size的值，可能变大或变小若干。通过调用对应次数的allocate或deallocate更新空间。
* **clear**：调用size次deallocate，逐次将原来的空间全部释放。

list的地址不连续，且每次操作的空间均为单个大小。针对这一特点，使用STL的allocator提供的allocate/deallocate，需要十分频繁地调用new与delete，造成很大的性能损耗。而且更容易引入内存碎片。对于这一容器，我们可以利用内存池模型明显优化其性能。

#### 管理模型

我们使用内存池为list分配空间的管理模式如下。最初，我们的allocator为list对象提供一定大小的大区块（chunk），并将当前的chunk作为内存资源的顶层管理者。结合如上介绍的内存池原理，每次进行allocate，我们从内存池中分配一个单位的内存。当我们无法从当前的内存池获得空间时，我们增加一个新的chunk，并将这个chunk作为新的顶层管理者。注意，无论哪一个chunk的一个block被释放，我们的顶层chunk都会将其添加到一个新的free block中。而且，由于list中的元素地址是不连续的。因此，我们每次实际可使用的最大block为所有chunk之和。这样既大幅度减少了new和delete的频率，又让前面被释放掉的block得到重新利用的机会。从理论上分析，内存池结构可以大大提高list的分配效率。但是由于memorypool本身实现的规定，我们使用的元素大小应大于sizeof(void*)，以获得十分显著的优化效果。否则针对更高效的利用内存，我们也更推荐使用标准库的allocator。

![list_pool](./figure/list_pool.png)

<center font = "time new roman";style="font-size:14px;color:#000000;">figure: example for list allocated by memory pool</center> 

因此，我们优化的核心是

* 预留内存块留作使用，避免频繁的new/delete操作。
* 及时可重用化释放的内存空间，提高内存利用率。

#### 配置器实现

实现部分为核心功能allocate以及deallocate。allocate中，我们通过一个vector结构`mpools`（事实上从测试结果看来，这里的时间是可接受的）保存内存区使用的所有chunk。其中，chunk_size为我们给定的一个chunk中block的个数。当内存池尚且剩余空间时，我们从原有空间分配一块block给list使用。否则我们增加一块新的chunk。在deallocate中，我们则可以通过free给当前chunk增加空闲资源。	

```c++
// allocate
pointer allocate(size_type n)
{
    if (_mpools.empty() == true) {
        // first allocator memory for user
        mem::PoolMemory* pool = new mem::PoolMemory(sizeof(T), chunk_size);
        _mpools.push_back(pool);
        // return a pointer to memory resource.
        pointer ptr = static_cast<pointer>(pool->get());
        return ptr;
    }
    else {
        mem::PoolMemory* pool = _mpools.back();
        // we still have free space.
        if (pool->full() == false) {
            // return a pointer to memory resource directly.
            pointer ptr = static_cast<pointer>(pool->get());
            return ptr;
        }
        // free space has been run out.
        else {
            //we need to allocate a new one by expand.
            pool = new mem::PoolMemory(sizeof(T), chunk_size);
            _mpools.push_back(pool);
            //return a new free block.
            pointer ptr = static_cast<pointer>(pool->get());
            return ptr;
        }
    }
}
// deallocate
void deallocate(pointer p, size_type n) {
    // set p free when it's deallocate.
    assert(p != nullptr);
    if (_mpools.empty() == false) {
        // set p free and allocate it to the top chunk.
        mem::PoolMemory* pool = _mpools.back();
        void* fblock = static_cast<void*>(p);
        pool->free(fblock);
    }
}
private:
	std::vector<mem::PoolMemory*> _mpools; // memory resource for management
```

#### 测试结果

我们测试了一些情况下list::allocator和std::allocator对于list的相关操作的效率对比。

编译环境为**MSVC Release x64**。数据为随机生成。因此时间取记录并求多次平均值。

| 平均时间(my_allocator)/s | 平均时间(std_allocator)/s | 备注                                                         |
| ------------------------ | ------------------------- | ------------------------------------------------------------ |
| 0.277                    | 0.004                     | 创建一个初始大小为10000的空list。                            |
| 2.106                    | 6.395                     | 进行10000个list的随机大小的创建。                            |
| 2.852                    | 8.052                     | 进行10000个list的随机大小的创建，并删除四分之一的元素，再次创建二分之一。 |
| 3.756                    | 11.286                    | 进行10000个list的随机大小的交替创建和删除，最终选择后1000个list进行resize和pop操作。 |
| 15.563                   | 35.368                    | 进行20000个list的随机大小的创建和删除，最终选择后3000个list进行resize和pop操作。再选择3000个list进行clear操作。 |

对于更大的测试数据，由于std的运行时间相对过长，且即使对于STL的allocator，内存使用也较为紧张。因此略去。从以上测试数据可以看到，利用内存池结构实现的allocator效率十分明显的成倍高于标准库。虽然由于开始分配较多的chunk，我们需要相对长一些的时间创建一个list。但比起我们之后对于list的各种调整size的操作，些许的时间花销相比优化明显是微不足道的。而且根据内存池结构特点，在进行size减少的删除操作后，我们对于内存的利用更加紧凑，可以十分地有效避免内存碎片的出现。

但相对分析，使用poolmemory引入的代价也是不容忽视的。首先，由于block本身增加了内存的消耗量，再加上按chunk分配，有可能会导致部分残存的少于一个chunk大小的内存无法使用。这里，chunk大小的适当选取就显得尤为重要。但相比我们获得的时间上的如此明显的优化，我们增加的内存用量也在可允许范围内。

### vector

#### 接口分析

相比list，**vector**对于内存的管理更为复杂。其主要区别于list对于内存管理的特点是：

* vector中的元素的内存地址应该是连续的。
* 当vector调用allocate时，其并非调用单块内存，或增加一块附加内存，而是要重新返回一块可以容纳n个element的内存区块。
* 大多数情况（操作）下，调用allocate得到的是一块更大的内存。而非更小的内存。

其中，我们常用的一些与size进行改变相关的操作接口如下：

* **push_back**：size的值增加一。当vector的容量（capacity）不再能容纳时，先调用allocate得到一块新的更大的空间。再调用deallocate释放原来的空间。这里新的更大空间的关系近似于幂次关系。这一类操作，只会得到更大的内存块。
* **pop_back**：size的值减去一。但不调用allocate和deallocate，因为空间对于删除后的vector一定是充足的。
* **resize**：resize改变size的值。但此处不同的是，凡是我们resize的空间，小于当前vector的capacity，vector不会调用allocate和deallocate。只有resize一片更大的内存，vector才会调用allocate和deallocate。例如。当我们的vector本来size为13时，我们依次进行resize为5、13，vector并不会调用allocate和deallocate。而如果调用的resize大于13，则会调用。因此，这一类操作，只会得到更大的内存块。
* **reserve**：resize改变capacity的值，但不改变size。因此只有新的容量大于旧的容量时，才会更新空间。当传入参数小于size，此方法不做任何事情。效果其实是与resize有类似之处的。同样，这一操作只可能得到更大的内存块。
* **clear**：size的值重置为0。但不调用allocate和deallocate。
* **shrink_to_fit**：c++11后新增特性。可以将vector中空闲的内存释放。也就是将capacity降至size。通过调用allocate/dellocate实现。因此，这一类操作，只会得到更小的内存块。

因此，我们可以看到，我们需要allocate给vector一块连续的空间，并将原来的空间deallocate。此外，在绝大多数情况下，allocate的空间都大于原来的空间。

#### 管理模型

我们思考，利用尽可能使用重复内存这一点，来提高我们对于内存的利用效率。然而这一点实际上对于vector是不容易实现的。基于vector的空间分配原理。因为其地址连续化，我们其实较难通过简单的内存池结构，利用前面碎片化的空间，这样很容易导致空间的覆盖。此外，在绝大多数情况下，我们需要allocate的是一块更大的空间。如果我们考虑保留之前分配过的空间，预留给vector后续要更小空间使用，减少new的次数。然而从上面的操作我们可以看到，我们实际上很少遇到需要一块更少内存的情况。反而这部分内存始终很难使用，这和我们的初衷相反。并不可行。

由于vector的capacity增长近似于指数，因此我们需要做的其实是对数次的allocate和deallocate。因此，我们想对于vector进行优化的主要方案，是尽可能预留合适的空间，这样可以减少allocate和deallocate被调用时，new和delete引入的时间。

在这个想法基础上，一种实现方案是，我们可以使用堆栈式内存结构，利用其分配和释放速度极快的特点。我们分配一个相对于当前vector所需，较为富裕的空间。在当前内存资源中如果无法获得足够空间，那么我们可以开辟新的内存空间。再进行下一轮操作。这样allocate的次数进一步减少，且对于分配空间，堆栈式内存结构表现更为迅速优秀。但应该注意，分配新空间的大小如果过大，那么这一次allocate的时间其实会引入更大的代价。尤其是当capacity到后期很大时，这会导致较大程度的浪费。选取内存资源的大小在适当的范围内显然是必要的。

#### 配置器实现



#### 测试结果



### allocator_trait

allocator_trait作为标准库容器使用allocator的一个接口，其本身具有丰富的功能，并且可以保证，对于任何容器引入的任何类，traits都可以转换成对应的类型进行处理。在c++17版本，由于allocator_trait实现了allocator的address、construct、destruct等多种功能，allocator中的这些接口已经不推荐使用。这些接口则在c++20中被删除。因此，为了更好的版本兼容，另外实现了一个较为简单的**allocator_trait**部分，其同时具备返回element的地址，以及建构和析构功能。

```c++
#pragma once
namespace trait {
template<typename T>
class Allocator_Traits {
public:
    template<typename U>
    struct rebind {
        typedef Allocator_Traits<U> other;
    };
    explicit Allocator_Traits() {}
    explicit Allocator_Traits(Allocator_Traits const&) {}
    template <typename U>
    explicit Allocator_Traits(Allocator_Traits<U> const&) {}
    ~Allocator_Traits() {}
    // address
    inline T* address(T& r) { return &r; }
    inline T const* address(T const& r) { return &r; }
    // construct(deprecated in C++17)
    template< typename U, typename... Args >
    void construct(U* p, Args&&... args) {
        new(p) U(std::forward<Args>(args)...);
    }
    // destruct
    template< typename U >
    void destroy(U* p) {
        p->~U();
    }
};
}
```

