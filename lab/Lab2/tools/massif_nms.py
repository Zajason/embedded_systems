#!/usr/bin/env python3
"""Heap of nms() at the peak of a Massif run.

    python3 tools/massif_nms.py m.out [--func nms]

Massif's heap tree lists, for every allocation site, the call stack from the allocating
function (operator new, malloc) outwards. A block belongs to nms() if nms() is somewhere on
its stack. We walk the tree of the (peak) snapshot and, on every path, take the FIRST node
whose function name matches, then do not descend further (its children are parts of it).
That is the "add up the lines that mention nms" of the assignment, without counting the
same bytes twice at different depths.
"""
import re
import sys

NODE = re.compile(r"^(\s*)n(\d+): (\d+) (.*)$")


def peak_tree(path):
    lines = open(path).read().splitlines()
    for i, l in enumerate(lines):
        if l == "heap_tree=peak":
            start = i + 1
            break
    else:
        sys.exit("no peak snapshot in " + path)
    # snapshot header (mem_heap_B etc.) is just above heap_tree
    head = {}
    for l in lines[start - 6:start - 1]:
        if "=" in l:
            k, v = l.split("=", 1)
            head[k] = v
    nodes = []
    for l in lines[start:]:
        m = NODE.match(l)
        if not m:
            break
        nodes.append((len(m.group(1)), int(m.group(3)), m.group(4)))
    return head, nodes


def nms_bytes(nodes, pattern):
    rx = re.compile(pattern)
    total, hits, skip_below = 0, [], None
    for depth, size, desc in nodes:
        if skip_below is not None and depth > skip_below:
            continue
        skip_below = None
        fn = desc.split(": ", 1)[-1] if ": " in desc else desc
        if rx.search(fn):
            total += size
            hits.append((size, desc))
            skip_below = depth
    return total, hits


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    pattern = r"\bnms(_orig)?\("
    if "--func" in sys.argv:
        pattern = sys.argv[sys.argv.index("--func") + 1]
    head, nodes = peak_tree(sys.argv[1])
    total, hits = nms_bytes(nodes, pattern)
    heap = int(head.get("mem_heap_B", 0))
    extra = int(head.get("mem_heap_extra_B", 0))
    print(f"peak heap (useful) : {heap:>10,} B")
    print(f"peak heap (extra)  : {extra:>10,} B  (malloc headers/alignment, all sites)")
    for size, desc in hits:
        print(f"  {size:>10,} B  {desc[:110]}")
    print(f"nms() at peak      : {total:>10,} B  ({total/1024:.1f} KiB)")


if __name__ == "__main__":
    main()
