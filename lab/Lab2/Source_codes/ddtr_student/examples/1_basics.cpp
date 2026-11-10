#include <ddtr_library>
#include <iostream>

// One vector, with the default allocator (glibc malloc).
int main() {
    ddtr::vector<int> values;
    values.push_back(10);
    values.push_back(20);
    values.push_back(30);

    values.write(1, 25);             // or values[1] = 25
    values[2] = values.read(0) + 5;  // read(i) returns a copy, values[i] a reference

    for (auto it = values.begin(); it != values.end(); ++it)
        *it *= 2;

    for (std::size_t i = 0; i < values.size(); ++i)
        std::cout << values[i] << ' ';
    std::cout << '\n';
}
