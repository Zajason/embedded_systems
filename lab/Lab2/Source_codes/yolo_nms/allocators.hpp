#ifndef ALLOCATORS_HPP
#define ALLOCATORS_HPP
// The allocators. postprocess.cpp includes this file, and so does main.cpp with -DALLOC_STATS.
// worst-fit is a complete example; the other four are for you to implement.
//
//   ./build.sh <ddt> arena
//   ./build.sh <ddt> first-fit
//   ./build.sh <ddt> best-fit
//   ./build.sh <ddt> worst-fit
//
// Add -DALLOC_STATS to print the footprint of the allocator. Do not use it in the runs that
// measure time.
#include "allocators/arena_allocator.hpp"
#include "allocators/first_fit_allocator.hpp"
#include "allocators/best_fit_allocator.hpp"
#include "allocators/worst_fit_allocator.hpp"

#endif
