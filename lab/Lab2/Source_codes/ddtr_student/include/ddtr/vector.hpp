#pragma once
#include "definitions.h"

namespace ddtr {
template<class T, class Allocator = malloc_allocator>
using vector = typename ddt_helper<T, Allocator>::VECTOR;
}
