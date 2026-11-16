// Random malloc/free against each allocator. Every live block is filled with its own byte
// pattern and checked before it is freed, so overlapping blocks or a broken free list show up.
//   g++ -std=c++17 -O2 -I.. test/alloc_stress.cpp -o bin/alloc_stress && ./bin/alloc_stress
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>
#include <vector>
#include "allocators.hpp"

template <class A>
static bool stress(const char* name) {
    struct live { unsigned char* p; std::size_t n; unsigned char v; };
    std::mt19937 rng(1);
    std::vector<live> blocks;
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 200000; i++) {
            bool alloc = blocks.empty() || rng() % 100 < 55;
            if (alloc) {
                std::size_t n = rng() % 4 == 0 ? rng() % 20000 : rng() % 300;
                auto* p = (unsigned char*)A::malloc(n);
                if ((std::uintptr_t)p % 16) { std::printf("%s: misaligned\n", name); return false; }
                unsigned char v = rng();
                std::memset(p, v, n);
                blocks.push_back({p, n, v});
            } else {
                std::size_t k = rng() % blocks.size();
                live b = blocks[k];
                for (std::size_t j = 0; j < b.n; j++)
                    if (b.p[j] != b.v) { std::printf("%s: block overwritten\n", name); return false; }
                A::free(b.p);
                blocks[k] = blocks.back();
                blocks.pop_back();
            }
        }
        for (auto& b : blocks) {  // free everything: the arena goes back to the start
            for (std::size_t j = 0; j < b.n; j++)
                if (b.p[j] != b.v) { std::printf("%s: block overwritten\n", name); return false; }
            A::free(b.p);
        }
        blocks.clear();
    }
    std::printf("%-10s OK\n", name);
    return true;
}

int main() {
    bool ok = stress<arena>("arena") & stress<first_fit>("first-fit") &
              stress<best_fit>("best-fit") & stress<worst_fit>("worst-fit");
    return ok ? 0 : 1;
}
