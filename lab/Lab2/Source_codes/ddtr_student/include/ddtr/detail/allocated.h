#ifndef DDTR_ALLOCATED_H
#define DDTR_ALLOCATED_H

#include <cstddef>

namespace DDTLibrary {
// Derive from this to make new/delete go through allocator A (see ddtr/allocator.hpp).
template <class A>
class allocated {
public:
    void* operator new(std::size_t sz) { return A::malloc(sz); }
    void operator delete(void* p) { A::free(p); }
    void* operator new[](std::size_t sz) { return A::malloc(sz); }
    void operator delete[](void* p) { A::free(p); }
};
}
#endif
