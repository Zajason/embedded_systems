#pragma once
#include "exceptions.hpp"
#include <new>
#include <type_traits>
#include <utility>

namespace DDTLibrary::detail {
// Top-down stable merge sort. One uninitialized half-sized buffer is shared by
// every merge; no temporary allocation escapes the container's allocator.
template<class T, class Allocator> class StableVectorSort {
    T *buffer_;
    std::size_t capacity_;
    static void assign(T *dst, T *src) {
        if(dst != src) *dst = std::move(*src);
    }
    void merge(T *data, std::size_t middle, std::size_t n) {
        std::size_t constructed=0;
        DDTR_TRY {
            for(; constructed<middle; ++constructed)
                ::new(static_cast<void*>(buffer_+constructed)) T(std::move_if_noexcept(data[constructed]));
            std::size_t left=0, right=middle, out=0;
            while(left<middle && right<n) {
                // Choose the left element on equivalence: stability matters.
                if(data[right]<buffer_[left])
                    assign(data+out++,data+right++);
                else
                    assign(data+out++,buffer_+left++);
            }
            while(left<middle) assign(data+out++,buffer_+left++);
        } DDTR_CATCH_ALL {
            while(constructed) buffer_[--constructed].~T();
            DDTR_RETHROW();
        }
        while(constructed) buffer_[--constructed].~T();
    }
    void recurse(T *data, std::size_t n) {
        if(n<2) return;
        const auto middle=n/2;
        recurse(data,middle); recurse(data+middle,n-middle);
        // Already ordered ranges need no moves (including all-equal ranges).
        if(!(data[middle]<data[middle-1])) return;
        merge(data,middle,n);
    }
public:
    explicit StableVectorSort(std::size_t n):
        buffer_(static_cast<T*>(Allocator::malloc(((n+1)/2)*sizeof(T)))),
        capacity_((n+1)/2) {
        if(!buffer_) DDTR_THROW(std::bad_alloc());
    }
    ~StableVectorSort() { Allocator::free(buffer_); }
    void operator()(T *data,std::size_t n) { recurse(data,n); }
};
}
