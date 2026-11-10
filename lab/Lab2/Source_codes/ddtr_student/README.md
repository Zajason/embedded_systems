# DDTR

A header-only C++17 library of dynamic data types (DDTs): a vector, linked lists and
trees. They all have the same interface, so you can replace one with another by
changing a single line. You can also choose which allocator each one uses.

## Installation

The library is header-only, so there is nothing to install. Point the compiler at
`include/`:

```sh
g++ -std=c++17 -O2 -I path/to/ddtr_student/include main.cpp -o main
```

## Tutorial

### 1. A first program

```cpp
#include <ddtr_library>
#include <iostream>

int main() {
    ddtr::vector<int> values;
    values.push_back(10);
    values.push_back(30);
    values.push_back(20);

    values[0] = 15;
    std::cout << "first: " << values.read(0) << ", size: " << values.size() << '\n';

    for (auto it = values.begin(); it != values.end(); ++it)
        std::cout << *it << ' ';
    std::cout << '\n';
}
```

Compilatopm is done using `include`:

```sh
g++ -std=c++17 -O2 -Iinclude main.cpp -o main
./main
```

### 2. Changing the DDT

Give the type a name with `using`, and use only that name in your code:

```cpp
using Values = ddtr::vector<int>;

Values values;
values.push_back(10);
```

Now changing the DDT is a one-line change:

```cpp
using Values = ddtr::single_linked_list<int>;
using Values = ddtr::red_black_tree<int>;
using Values = ddtr::b_tree<int>;
```

The program stays the same. 
Trees keep their values sorted, so after `push_back(10)`, `push_back(30)` and `push_back(20)`, index 1 of a tree
holds 20, not 30.

### 3. Writing your own allocator

The second template argument is the allocator. An allocator is a struct with two
static functions, `malloc` and `free`. This one uses the normal `malloc`, but also
counts how many allocations the DDT makes:

```cpp
#include <ddtr_library>
#include <cstdlib>
#include <iostream>

struct counting_allocator {
    static inline int allocations = 0;

    static void* malloc(std::size_t size) {
        allocations++;
        return std::malloc(size);
    }
    static void free(void* pointer) {
        std::free(pointer);
    }
};

int main() {
    ddtr::single_linked_list<int, counting_allocator> values;
    for (int i = 0; i < 1000; ++i)
        values.push_back(i);

    std::cout << counting_allocator::allocations << " allocations\n";
}
```

This prints 1001: one node per value, plus the list's sentinel node. Try
`ddtr::vector` instead and you get 11, because the vector doubles its buffer when it
fills up (1, 2, 4, ..., 1024).

The DDT gets all of its memory through its allocator, so this is where you can plug in
a pool, an arena, or anything else you write.

### 4. Choosing both when compiling

Editing the source for every combination gets tedious. Instead, use a macro with a
default value:

```cpp
#include <ddtr_library>
#include <chrono>
#include <iostream>

#ifndef sel_dt_values
#define sel_dt_values vector
#endif
#ifndef sel_alloc_values
#define sel_alloc_values ddtr::malloc_allocator
#endif
using Values = ddtr::sel_dt_values<int, sel_alloc_values>;

int main() {
    auto start = std::chrono::steady_clock::now();

    Values values;
    for (int i = 0; i < 20000; ++i)
        values.push_back(i * 7 % 1000);

    long sum = 0;
    for (auto it = values.begin(); it != values.end(); ++it)
        sum += *it;

    while (!values.empty())
        values.pop_front();

    auto end = std::chrono::steady_clock::now();
    std::cout << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n";
}
```

and pick the DDT (and the allocator) with `-D`:

```sh
for dt in vector single_linked_list red_black_tree b_tree; do
    g++ -std=c++17 -O2 -Iinclude -Dsel_dt_values=$dt app.cpp -o app
    echo -n "$dt: "; ./app
done
```

```text
vector: 13.9 ms
single_linked_list: 0.97 ms
red_black_tree: 2.03 ms
b_tree: 1.92 ms
```

The vector loses here because every `pop_front` shifts all the remaining elements.
With a different workload the order changes, and that is the point: the best DDT
depends on how the program uses it.

### 5. Using an existing allocator

You do not have to write the allocator yourself. Any library with its own malloc and
free fits in the same two static functions. For [jemalloc](https://jemalloc.net),
save this as `jemalloc_allocator.hpp`:

```cpp
#include <cstddef>
#include <jemalloc/jemalloc.h>

struct jemalloc_allocator {
    static void* malloc(std::size_t size) { return mallocx(size ? size : 1, 0); }
    static void free(void* pointer) {
        if (pointer)
            dallocx(pointer, 0);
    }
};
```

and for [mimalloc](https://github.com/microsoft/mimalloc), as `mimalloc_allocator.hpp`:

```cpp
#include <cstddef>
#include <mimalloc.h>

struct mimalloc_allocator {
    static void* malloc(std::size_t size) { return mi_malloc(size); }
    static void free(void* pointer) { mi_free(pointer); }
};
```

Install the libraries (`sudo apt install libjemalloc-dev libmimalloc-dev` on Ubuntu).
Then, with the program from step 4, `-include` adds the header without editing
`app.cpp`, and `-l` links the library:

```sh
g++ -std=c++17 -O2 -Iinclude -include jemalloc_allocator.hpp \
    -Dsel_alloc_values=jemalloc_allocator app.cpp -o app -ljemalloc
g++ -std=c++17 -O2 -Iinclude -include mimalloc_allocator.hpp \
    -Dsel_alloc_values=mimalloc_allocator app.cpp -o app -lmimalloc
```

Note: On Linux, `-ljemalloc` also replaces the normal `malloc` for the whole program, so
`ddtr::malloc_allocator` becomes jemalloc too. Build the `malloc` baseline without it.

## Reference

### DDTs

| Name | |
| --- | --- |
| `vector` | Contiguous array. |
| `single_linked_list`, `double_linked_list` | Linked lists. |
| `roving_...`, `..._no_sentinel` | List variants: `roving_` remembers the last accessed node, `_no_sentinel` has no dummy node. They combine, e.g. `roving_double_linked_list_no_sentinel`. |
| `binary_search_tree`, `red_black_tree`, `avl_tree` | Binary search trees (only the last two are balanced). |
| `b_tree` | B-tree (from https://github.com/abseil/abseil-cpp). `b_tree_3`, `b_tree_8`, `b_tree_32`... set the values per node. |

### Operations

All DDTs have:

| | |
| --- | --- |
| `push_back(v)` | Add a value (trees put it in sorted order). |
| `pop_front()`, `pop_back()` | Remove the first / last value. |
| `read(i)`, `write(i, v)`, `v[i]` | Access by index. |
| `front()`, `back()` | First / last value. |
| `size()`, `empty()`, `clear()` | |
| `remove_value(v)` | Remove the first value equal to `v`. |
| `sort()` | Sort (trees are always sorted). |
| `begin()`, `end()`, `rbegin()`, `rend()` | Iterators. |
| `erase(it)` | Remove the value at an iterator. |
| `footprint()` | Bytes of memory the DDT uses. |

Vectors and lists also have `add(i, v)` and `remove(i)` (insert / remove at index `i`).
Trees also have `find(v)`, `insert_unique(v)` and `remove(v)`.

Two things to keep in mind:

- `read(i)` on a list or tree walks from the start, so a loop with `read(i)` over a list
  is O(n²). Use iterators.
- Writing to a tree with `write(i, v)` or `v[i]` must keep the values sorted.

## Examples

In `examples/`; build one with `g++ -std=c++17 -Iinclude examples/<name>.cpp`, or all
of them with `cmake -B build && cmake --build build`.

1. `1_basics.cpp`: one vector, accessed in each way.
2. `2_swap_ddt.cpp`: the same program on any DDT (`-Dsel_dt_scores=...`).
3. `3_btree_nodes.cpp`: prints a B-tree's nodes as values are added and removed.
4. `4_custom_allocator.cpp`: a counting allocator, comparing a vector and a list.
5. `5_bfs.cpp`: a breadth-first search with three DDTs, each chosen separately.

`include/ddtr/BTree.h` is a port of Abseil's B-tree, under the Apache 2.0 license
([LICENSE-abseil.txt](LICENSE-abseil.txt)).
