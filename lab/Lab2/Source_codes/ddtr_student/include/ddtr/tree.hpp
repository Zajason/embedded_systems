#pragma once
#include "definitions.h"

namespace ddtr {
template<class T, class Allocator = malloc_allocator>
using binary_search_tree = typename ddt_helper<T, Allocator>::BINARYTREE;
template<class T, class Allocator = malloc_allocator>
using red_black_tree = typename ddt_helper<T, Allocator>::REDBLACKTREE;
template<class T, class Allocator = malloc_allocator>
using avl_tree = typename ddt_helper<T, Allocator>::AVLTREE;
// B-tree ported from Abseil (see BTree.h); NodeSlots is the number of values per
// node (at least 3), or 0 for Abseil's default of 256-byte nodes.
template<class T, class Allocator = malloc_allocator, int NodeSlots = 0>
using b_tree = DDTLibrary::BTree<T, Allocator, NodeSlots>;
// Fixed node sizes, selectable by name like the other DDTs (-Dsel_dt_<name>=b_tree_8).
template<class T, class Allocator = malloc_allocator>
using b_tree_3 = b_tree<T, Allocator, 3>;
template<class T, class Allocator = malloc_allocator>
using b_tree_4 = b_tree<T, Allocator, 4>;
template<class T, class Allocator = malloc_allocator>
using b_tree_8 = b_tree<T, Allocator, 8>;
template<class T, class Allocator = malloc_allocator>
using b_tree_16 = b_tree<T, Allocator, 16>;
template<class T, class Allocator = malloc_allocator>
using b_tree_32 = b_tree<T, Allocator, 32>;
}
