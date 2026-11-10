#ifndef WORST_FIT_ALLOCATOR_HPP
#define WORST_FIT_ALLOCATOR_HPP
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>

// Worst fit allocator: a complete example. The free blocks are kept in a list sorted by
// address. malloc cuts each new block from the BIGGEST free block, and free puts the block
// back in the list and merges it with its free neighbours.
struct worst_fit {
    // In front of every block. 16 bytes, and all sizes are multiples of 16, so the memory
    // that malloc returns is aligned to 16 bytes.
    struct header {
        std::size_t size;  // of the whole block, header included
        header* next;      // next free block (only while this block is free)
    };

    static constexpr std::size_t MMAP_BYTES = 1 << 20;
    static inline header* free_list = nullptr;

    static void* malloc(std::size_t size) {
        std::size_t needed = (sizeof(header) + size + 15) / 16 * 16;

        header* biggest = free_list;
        for (header* b = free_list; b; b = b->next)
            if (b->size > biggest->size) biggest = b;

        // Keep room for one more header, so that the free block is never used up and never
        // has to be removed from the list.
        if (!biggest || biggest->size < needed + sizeof(header)) {
            std::size_t bytes = (needed + sizeof(header) + MMAP_BYTES - 1) / MMAP_BYTES * MMAP_BYTES;
            void* memory = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (memory == MAP_FAILED) {
                std::fprintf(stderr, "worst_fit: mmap of %zu bytes failed\n", bytes);
                std::abort();
            }
#ifdef ALLOC_STATS
            mmap_bytes += bytes;
#endif
            biggest = (header*)memory;
            biggest->size = bytes;
            insert(biggest);
        }

        // Cut the new block from the end, so the free block stays where it is in the list.
        biggest->size -= needed;
        header* block = (header*)((char*)biggest + biggest->size);
        block->size = needed;
#ifdef ALLOC_STATS
        malloc_bytes += needed;
        if (malloc_bytes > peak_malloc_bytes) peak_malloc_bytes = malloc_bytes;
#endif
        return block + 1;
    }

    static void free(void* pointer) {
        if (!pointer) return;
        header* block = (header*)pointer - 1;  // one header (16 bytes) back
#ifdef ALLOC_STATS
        malloc_bytes -= block->size;
#endif
        insert(block);
    }

    // Puts a block in the free list, in address order, and merges it with the free blocks
    // right before and after it.
    static void insert(header* block) {
        header* before = nullptr;
        header* after = free_list;
        while (after && after < block) {
            before = after;
            after = after->next;
        }
        block->next = after;
        if (before) before->next = block;
        else free_list = block;

        if (after && (char*)block + block->size == (char*)after) {
            block->size += after->size;
            block->next = after->next;
        }
        if (before && (char*)before + before->size == (char*)block) {
            before->size += block->size;
            before->next = block->next;
        }
    }

#ifdef ALLOC_STATS
    // All the bytes you got from mmap. With one huge mmap this is only the size you picked,
    // so map smaller pieces as you need them.
    static inline std::size_t mmap_bytes = 0;
    // The bytes of the blocks that are live now, and the most there ever were at the same time.
    static inline std::size_t malloc_bytes = 0;
    static inline std::size_t peak_malloc_bytes = 0;

    static void report_footprint() {
        std::printf("mmap_bytes=%zu malloc_bytes=%zu\n", mmap_bytes, peak_malloc_bytes);
    }
#endif
};

#endif
