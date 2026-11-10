#pragma once
#include <cstdlib>

namespace ddtr {

// The default allocator: glibc malloc and free.
//
// Every DDT allocates and frees all of its memory (vector buffers, list and tree nodes)
// through its Allocator argument, so an allocator is any type with these two static
// functions. See examples/3_custom_allocator.cpp for one of your own.
struct malloc_allocator {
    static void* malloc(std::size_t size) { return std::malloc(size); }
    static void free(void* pointer) { std::free(pointer); }
};

} // namespace ddtr
