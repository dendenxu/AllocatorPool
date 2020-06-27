#include <iostream>
#include <vector>
namespace mem
{
// ! This class can only be used when sizeof(uintptr_t) <= sizeof(T)
// actually it's not even recommended to use memory pool if your element size is quite small, the pointers would take more space than the actual elements!
// one possible usage for the allocator is that upon encountering a small sized element allocation request, it calls this pool memory resource to construct larger space, and savor the large block by itself
class PoolMemory
{
   public:
    PoolMemory(const std::size_t element_sz_bytes, const std::size_t num_elements)
        : m_element_sz_bytes(element_sz_bytes),
          m_num_elements(num_elements),
          m_total_num_elements(num_elements),
          m_pool_sz_bytes(num_elements * element_sz_bytes)
    {
        assert(sizeof(std::uintptr_t) <= m_element_sz_bytes);
        m_pmemory = new std::byte[m_pool_sz_bytes];                   // using char as byte
        m_ppfree_memory_list = reinterpret_cast<void **>(m_pmemory);  // treat list pointer as a pointer to pointer
        std::uintptr_t start_addr = reinterpret_cast<std::uintptr_t>(m_ppfree_memory_list);
        std::uintptr_t end_addr = start_addr + m_pool_sz_bytes;
        // construct the linked list from raw memory
        for (auto i = 0; i < m_total_num_elements; i++) {
            std::uintptr_t curr_addr = start_addr + i * m_element_sz_bytes;  // current element's address
            std::uintptr_t next_addr = curr_addr + m_element_sz_bytes;       // next element's address
            void **curr_mem = reinterpret_cast<void **>(curr_addr);
            if (next_addr >= end_addr) {
                *curr_mem = nullptr;
            } else {
                *curr_mem = reinterpret_cast<void *>(next_addr);
            }
        }
    }

    PoolMemory(const PoolMemory &alloc) = delete;           // copy constructor
    PoolMemory &operator=(const PoolMemory &rhs) = delete;  // copy-assignment operator
    PoolMemory(PoolMemory &&alloc) = delete;                // move constructor
    PoolMemory &operator=(PoolMemory &&rhs) = delete;       // move-assignment operator

    ~PoolMemory() { delete[] m_pmemory; }  // delete the pre-allocated memory pool chunk

    std::size_t element_size() { return m_element_sz_bytes; }              // return element size in byte
    std::size_t pool_size() { return m_pool_sz_bytes; }                    // return memory pool size in byte
    std::size_t free_list_size() { return m_num_elements; }                // return current number of free list inside the memory pool
    std::size_t count() { return m_total_num_elements - m_num_elements; }  // return the number of used space in the memory pool
    std::size_t empty() { return m_num_elements == 0; }                    // return whether the memory pool is empty

    // return a nullptr if the memory pool is already full
    void *get()
    {
        if (m_pmemory == nullptr) {
            std::cerr << "ERROR " << __FUNCTION__ << ": No memory was allocated to this pool" << std::endl;
            return nullptr;
        }
        if (m_ppfree_memory_list != nullptr) {
            m_num_elements--;  // decrement the number of elements

            void *pelement = static_cast<void *>(m_ppfree_memory_list);
            m_ppfree_memory_list = static_cast<void **>(*m_ppfree_memory_list);

            return pelement;
        } else {  // out of memory blocks
            std::cerr << "ERROR " << __FUNCTION__ << ": out of memory blocks" << std::endl;
            return nullptr;
        }
    }

    // make sure the pelement is one of the pointers that you get from this memory pool
    void free(void *pelement)
    {
        if (pelement == nullptr) {
            // do nothing if we're freeing a nullptr
            return;
        }

        if (m_pmemory == nullptr) {
            std::cerr << "ERROR " << __FUNCTION__ << ": No memory was allocated to this pool" << std::endl;
            return;
        }

        m_num_elements++;                       // increment the number of elements
        if (m_ppfree_memory_list == nullptr) {  // the free list is empty
            m_ppfree_memory_list = static_cast<void **>(pelement);
            *m_ppfree_memory_list = nullptr;
        } else {
            void **ppreturned_block = m_ppfree_memory_list;
            m_ppfree_memory_list = static_cast<void **>(pelement);
            *m_ppfree_memory_list = static_cast<void *>(ppreturned_block);
        }
    }

   private:
    std::byte *m_pmemory;              // pointer to the first address of the pool, used to relase all the memory
    std::size_t m_pool_sz_bytes;       //the size in bytes of the pool
    std::size_t m_element_sz_bytes;    // size in bytes of each element
    std::size_t m_num_elements;        // number of elements
    std::size_t m_total_num_elements;  // total number of elements
    void **m_ppfree_memory_list;       // pointer to pointer, used to point to the head of the free list
};
}  // namespace mymem