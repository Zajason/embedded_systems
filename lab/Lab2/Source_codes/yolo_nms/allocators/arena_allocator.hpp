#ifndef ARENA_ALLOCATOR_HPP
#define ARENA_ALLOCATOR_HPP
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>

// Arena allocator: malloc moves a position forward through the memory, and free never gives a
// single block back. When all the blocks have been freed, the position goes back to the start.
struct arena {
    // The memory comes from mmap in chunks (1 MB, or more for a bigger request), kept in a
    // list in the order they were mapped. A chunk is never unmapped: when the position goes
    // back to the start, the same chunks are used again.
    struct chunk {
        std::size_t size;  // of the whole chunk, this header included
        chunk* next;       // next chunk in the list
    };
    // Sizes are rounded up to 16 and the chunk header is 16 bytes, so every block that malloc
    // returns is aligned to 16 bytes, like glibc's. Blocks have no header of their own.
    static_assert(sizeof(chunk) == 16, "chunk header must be 16 bytes");

    static constexpr std::size_t MMAP_BYTES = 1 << 20;
    static inline chunk* first = nullptr;    // first chunk ever mapped
    static inline chunk* last = nullptr;     // last chunk, where new chunks are appended
    static inline chunk* current = nullptr;  // chunk the position is in
    static inline std::size_t offset = 0;    // the position, from the start of current
    static inline std::size_t live = 0;      // blocks malloc'd and not yet freed

    static void* malloc(std::size_t size) {
        std::size_t needed = (size + 15) / 16 * 16;
        if (needed == 0) needed = 16;

        // Move on to the next chunks until one has room; map a new one if none has.
        while (!current || offset + needed > current->size) {
            if (current && current->next) {
                current = current->next;
            } else {
                std::size_t bytes = (sizeof(chunk) + needed + MMAP_BYTES - 1) / MMAP_BYTES * MMAP_BYTES;
                void* memory = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (memory == MAP_FAILED) {
                    std::fprintf(stderr, "arena: mmap of %zu bytes failed\n", bytes);
                    std::abort();
                }
#ifdef ALLOC_STATS
                mmap_bytes += bytes;
#endif
                chunk* c = (chunk*)memory;
                c->size = bytes;
                c->next = nullptr;
                if (last) last->next = c;
                else first = c;
                last = c;
                current = c;
            }
            offset = sizeof(chunk);
        }

        void* block = (char*)current + offset;
        offset += needed;
        ++live;
#ifdef ALLOC_STATS
        malloc_bytes += needed;
        if (malloc_bytes > peak_malloc_bytes) peak_malloc_bytes = malloc_bytes;
#endif
        return block;
    }

    static void free(void* pointer) {
        if (!pointer) return;
        // No single block is given back. When the last live block goes, the whole arena is
        // empty and the position goes back to the start of the first chunk.
        if (--live == 0) {
            current = first;
            offset = sizeof(chunk);
#ifdef ALLOC_STATS
            malloc_bytes = 0;
#endif
        }
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
