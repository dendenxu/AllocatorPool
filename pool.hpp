#include <iostream>
namespace mem
{
// ! This class can only be used when sizeof(uintptr_t) <= sizeof(T)
// actually it's not even recommended to use memory pool if your block size is quite small, the pointers would take more space than the actual blocks!
// one possible usage for the allocator is that upon encountering a small sized block allocation request, it calls this pool memory resource to construct larger space, and savor the large block by itself
class PoolMemory
{
   public:
    PoolMemory(const std::size_t block_sz_bytes, const std::size_t num_blocks);

    PoolMemory(const PoolMemory &alloc) = delete;           // delete copy constructor
    PoolMemory &operator=(const PoolMemory &rhs) = delete;  // delete copy-assignment operator
    PoolMemory(PoolMemory &&alloc) = delete;                // delete move constructor
    PoolMemory &operator=(PoolMemory &&rhs) = delete;       // delete move-assignment operator

    ~PoolMemory() { delete[] m_pmemory; }  // delete the pre-allocated memory pool chunk

    std::size_t block_size() { return m_block_sz_bytes; }                  // return block size in byte
    std::size_t pool_size() { return m_pool_sz_bytes; }                    // return memory pool size in byte
    std::size_t free_count() { return m_free_num_blocks; }                 // return number of free blocks inside the memory pool
    std::size_t size() { return m_total_num_blocks - m_free_num_blocks; }  // return the number of used space in the memory pool
    std::size_t capacity() { return m_total_num_blocks; }                  // return total number of blocks that this pool can hold
    bool empty() { return m_free_num_blocks == m_total_num_blocks; }       // return whether the memory pool is empty
    bool full() { return m_free_num_blocks == 0; }                         // return whether the memory pool is full

    // return a nullptr if the memory pool is already full
    // else this returns a pointer to an block whose size(still raw memory) is m_block_sz_bytes
    void *get();

    // make sure the pblock is one of the pointers that you get from this memory pool
    void free(void *pblock);

   private:
    /** Current size of a memory pool variable should be 48 bytes
     *  considering 8 byte for one pointer and size_t on my machine
     */
    std::byte *m_pmemory;            // pointer to the first address of the pool, used to relase all the memory
    void **m_phead;                  // pointer to pointer, used to point to the head of the free list
    std::size_t m_pool_sz_bytes;     //the size in bytes of the pool
    std::size_t m_block_sz_bytes;    // size in bytes of each block
    std::size_t m_free_num_blocks;   // number of blocks
    std::size_t m_total_num_blocks;  // total number of blocks
};

class ByteMemory
{
   public:
    ByteMemory(const std::size_t size) : m_total_size(size), m_index(0) { m_pmemory = new std::byte[size]; }
    ByteMemory(const std::size_t size, std::byte *pointer) : m_pmemory(pointer), m_index(0), m_total_size(size) {}

    ByteMemory(const ByteMemory &alloc) = delete;           // delete copy constructor
    ByteMemory &operator=(const ByteMemory &rhs) = delete;  // delete copy-assignment operator
    ByteMemory(ByteMemory &&alloc) = delete;                // delete move constructor
    ByteMemory &operator=(ByteMemory &&rhs) = delete;       // delete move-assignment operator

    ~ByteMemory() { delete[] m_pmemory; }  // delete the pre-allocated byte chunk chunk

    std::size_t free_count() { return m_total_size - m_index; }  // return number of free blocks inside the byte chunk
    std::size_t size() { return m_index; }                       // return the number of used space in the byte chunk
    std::size_t capacity() { return m_total_size; }              // return total number of blocks that this pool can hold
    bool empty() { return m_index == 0; }                        // return whether the byte chunk is empty
    bool full() { return m_index == m_total_size; }              // return whether the byte chunk is full

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
    void free(std::size_t size)
    {
        if (m_index < size) {  // this should never happen
            std::cerr << "[ERROR] You can only give back what you've taken away." << std::endl;
        } else {
            m_index -= size;
        }
    }

   private:
    std::byte *m_pmemory;      // pointer to the byte array
    std::size_t m_index;       // current index of the byte array
    std::size_t m_total_size;  // total number of blocks
};

}  // namespace mem