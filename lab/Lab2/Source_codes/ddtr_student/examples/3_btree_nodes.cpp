#include <ddtr_library>
#include <iostream>

// A B-tree keeps several sorted values in each node. Here a node holds at most 3,
// so a short run already shows the tree growing and shrinking. print() writes the
// shape as nested brackets: "[[5 10] 20 [25 30]]" is a root holding 20 with a leaf
// of 5 and 10 on its left and a leaf of 25 and 30 on its right.
int main() {
    ddtr::b_tree_3<int> values;

    std::cout << "insert\n";
    for (int v : {30, 10, 20, 40, 50, 25, 35, 45, 5, 15, 12}) {
        values.push_back(v);
        std::cout << "  " << v << "\t";
        values.print(std::cout);
        std::cout << '\n';
    }

    std::cout << "rank 3 holds " << values.read(3) << "\n\nremove\n";
    for (int v : {10, 20, 25, 30, 40}) {
        values.remove_value(v);
        std::cout << "  " << v << "\t";
        values.print(std::cout);
        std::cout << '\n';
    }
}
