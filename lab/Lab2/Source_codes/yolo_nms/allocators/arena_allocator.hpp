#ifndef ARENA_ALLOCATOR_HPP
#define ARENA_ALLOCATOR_HPP
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>

// Arena allocator: malloc moves a position forward through the memory, and free never gives a
// single block back. When all the blocks have been freed, the position goes back to the start.
struct arena {
    // TODO: your state, as static inline members

    static void* malloc(std::size_t size) {
        // TODO
#ifdef ALLOC_STATS
        // TODO
#endif
        std::fprintf(stderr, "arena::malloc is not implemented yet\n");
        std::abort();
    }

    static void free(void* pointer) {
        // TODO
#ifdef ALLOC_STATS
        // TODO
#endif
    }

#ifdef ALLOC_STATS
    // All the bytes you got from mmap. With one huge mmap this is only the size you picked,
    // so map smaller pieces as you need them.
    static inline std::size_t mmap_bytes = 0;
    // The bytes used since the position was last at the start, and the most there ever were.
    static inline std::size_t malloc_bytes = 0;
    static inline std::size_t peak_malloc_bytes = 0;

    static void report_footprint() {
        std::printf("mmap_bytes=%zu malloc_bytes=%zu\n", mmap_bytes, peak_malloc_bytes);
    }
#endif
};

#endif
