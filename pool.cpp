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

    /** We're using uintptr_t to perform arithmetic operations with confidence
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
    // if (pblock == nullptr) {
    //     // do nothing if we're freeing a nullptr
    //     // although this situation is declared undefined in C++ Standard
    //     return;
    // }

    if (m_pmemory == nullptr) {  // this should not happen
        std::cerr << "ERROR " << __FUNCTION__ << ": No memory was allocated to this pool" << std::endl;
        return;
    }

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