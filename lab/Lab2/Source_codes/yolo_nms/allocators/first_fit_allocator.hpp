#ifndef FIRST_FIT_ALLOCATOR_HPP
#define FIRST_FIT_ALLOCATOR_HPP
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>

// First fit allocator: malloc takes the FIRST free block that is big enough.
struct first_fit {
    // In front of every block, as in worst_fit. 16 bytes, and all sizes are multiples of 16,
    // so the memory that malloc returns is aligned to 16 bytes.
    struct header {
        std::size_t size;  // of the whole block, header included
        header* next;      // next free block (only while this block is free)
    };

    static constexpr std::size_t MMAP_BYTES = 1 << 20;
    // A free block smaller than this is not worth keeping: header + 16 bytes of data.
    static constexpr std::size_t MIN_BLOCK = sizeof(header) + 16;
    // Free blocks, sorted by address. Allocated blocks are in no list.
    static inline header* free_list = nullptr;

    static void* malloc(std::size_t size) {
        std::size_t needed = (sizeof(header) + size + 15) / 16 * 16;

        // The first free block in address order that is big enough.
        header* before = nullptr;
        header* found = free_list;
        while (found && found->size < needed) {
            before = found;
            found = found->next;
        }

        if (!found) {
            std::size_t bytes = (needed + MMAP_BYTES - 1) / MMAP_BYTES * MMAP_BYTES;
            void* memory = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (memory == MAP_FAILED) {
                std::fprintf(stderr, "first_fit: mmap of %zu bytes failed\n", bytes);
                std::abort();
            }
#ifdef ALLOC_STATS
            mmap_bytes += bytes;
#endif
            header* fresh = (header*)memory;
            fresh->size = bytes;
            insert(fresh);  // may merge it with the free block at the end of the last piece
            before = nullptr;
            found = free_list;
            while (found && found->size < needed) {
                before = found;
                found = found->next;
            }
        }

        header* block;
        if (found->size - needed >= MIN_BLOCK) {
            // Cut the new block from the end, so the free block keeps its place in the list.
            found->size -= needed;
            block = (header*)((char*)found + found->size);
            block->size = needed;
        } else {
            // What would be left is too small to be a block: hand out the whole block.
            if (before) before->next = found->next;
            else free_list = found->next;
            block = found;
        }
#ifdef ALLOC_STATS
        malloc_bytes += block->size;
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
