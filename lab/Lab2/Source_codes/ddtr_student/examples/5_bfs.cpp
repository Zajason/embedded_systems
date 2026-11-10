#include <ddtr_library>
#include <iostream>

// Breadth-first search over a small graph, with three DDTs. Each one gets its own
// DDT and allocator at build time:
//   edges     written once, then only read      vector or any list
//   frontier  push_back at the back, pop_front  vector or any list
//   visited   insert_unique per neighbour       any tree
// e.g. a vector frontier and a B-tree visited set:
//   -Dsel_dt_frontier=vector -Dsel_dt_visited=b_tree
// and -Dsel_alloc_<name>=<your allocator> to change the allocator of one of them.
#ifndef sel_dt_edges
#define sel_dt_edges vector
#endif
#ifndef sel_dt_frontier
#define sel_dt_frontier single_linked_list
#endif
#ifndef sel_dt_visited
#define sel_dt_visited red_black_tree
#endif
#ifndef sel_alloc_edges
#define sel_alloc_edges ddtr::malloc_allocator
#endif
#ifndef sel_alloc_frontier
#define sel_alloc_frontier ddtr::malloc_allocator
#endif
#ifndef sel_alloc_visited
#define sel_alloc_visited ddtr::malloc_allocator
#endif
using Edges = ddtr::sel_dt_edges<int, sel_alloc_edges>;
using Frontier = ddtr::sel_dt_frontier<int, sel_alloc_frontier>;
using Visited = ddtr::sel_dt_visited<int, sel_alloc_visited>;

// Node n links to links[first[n]] .. links[first[n + 1] - 1].
const int first[] = {0, 2, 4, 6, 7, 8, 9, 10, 10};
const int links[] = {1, 2, 3, 4, 4, 5, 6, 6, 7, 7};

int main() {
    Edges edges;
    for (int to : links)
        edges.push_back(to);

    Frontier frontier;
    Visited visited;
    frontier.push_back(0);
    visited.insert_unique(0);

    while (frontier.size() > 0) {
        int node = frontier.read(0);
        frontier.pop_front();
        std::cout << node << ' ';
        for (int e = first[node]; e < first[node + 1]; ++e) {
            int next = edges.read(e);
            if (visited.insert_unique(next).second)
                frontier.push_back(next);
        }
    }
    std::cout << '\n';
}
