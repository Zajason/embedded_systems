#pragma once
#include "definitions.h"

namespace ddtr {
template<class T, class Allocator = malloc_allocator>
using single_linked_list = typename ddt_helper<T, Allocator>::SLLLIST;
template<class T, class Allocator = malloc_allocator>
using double_linked_list = typename ddt_helper<T, Allocator>::DLLLIST;
template<class T, class Allocator = malloc_allocator>
using roving_single_linked_list = typename ddt_helper<T, Allocator>::SLLROVINGLIST;
template<class T, class Allocator = malloc_allocator>
using roving_double_linked_list = typename ddt_helper<T, Allocator>::DLLROVINGLIST;
template<class T, class Allocator = malloc_allocator>
using single_linked_list_no_sentinel = typename ddt_helper<T, Allocator>::SLLLIST_NOSENTINEL;
template<class T, class Allocator = malloc_allocator>
using double_linked_list_no_sentinel = typename ddt_helper<T, Allocator>::DLLLIST_NOSENTINEL;
template<class T, class Allocator = malloc_allocator>
using roving_single_linked_list_no_sentinel = typename ddt_helper<T, Allocator>::SLLNOSENTINELROVINGLIST;
template<class T, class Allocator = malloc_allocator>
using roving_double_linked_list_no_sentinel = typename ddt_helper<T, Allocator>::DLLNOSENTINELROVINGLIST;
}
