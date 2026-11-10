#include <ddtr_library>
#include <cstdlib>
#include <iostream>

// An allocator is any type with static malloc(size) and free(pointer); a DDT takes
// it as its second template argument. This one forwards to malloc and counts what
// passes through it. The Id makes counting_allocator<1> and counting_allocator<2>
// different types, so each DDT gets its own counters.
template<int Id>
struct counting_allocator {
    static inline std::size_t allocations = 0, frees = 0, bytes = 0;

    static void* malloc(std::size_t size) {
        ++allocations;
        bytes += size;
        return std::malloc(size);
    }
    static void free(void* pointer) {
        if (pointer) ++frees;
        std::free(pointer);
    }
};

template<class Allocator>
void report(const char* name) {
    std::cout << name << ": " << Allocator::allocations << " allocations, "
              << Allocator::frees << " frees, " << Allocator::bytes << " bytes\n";
}

// The same 1000 push_backs and 500 pop_fronts on a vector and on a list.
template<class DDT>
void run() {
    DDT values;
    for (int i = 0; i < 1000; ++i)
        values.push_back(i);
    for (int i = 0; i < 500; ++i)
        values.pop_front();
}

int main() {
    using VectorAlloc = counting_allocator<1>;
    using ListAlloc = counting_allocator<2>;
    run<ddtr::vector<int, VectorAlloc>>();
    run<ddtr::single_linked_list<int, ListAlloc>>();
    report<VectorAlloc>("vector");
    report<ListAlloc>("single_linked_list");
}
