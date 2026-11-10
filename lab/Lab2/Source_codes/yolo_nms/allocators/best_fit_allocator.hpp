#ifndef BEST_FIT_ALLOCATOR_HPP
#define BEST_FIT_ALLOCATOR_HPP
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>

// Best fit allocator: malloc takes the SMALLEST free block that is big enough.
struct best_fit {
    // TODO: your state, as static inline members

    static void* malloc(std::size_t size) {
        // TODO
#ifdef ALLOC_STATS
        // TODO
#endif
        std::fprintf(stderr, "best_fit::malloc is not implemented yet\n");
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
    // The bytes of the blocks that are live now, and the most there ever were at the same time.
    static inline std::size_t malloc_bytes = 0;
    static inline std::size_t peak_malloc_bytes = 0;

    static void report_footprint() {
        std::printf("mmap_bytes=%zu malloc_bytes=%zu\n", mmap_bytes, peak_malloc_bytes);
    }
#endif
};

#endif
