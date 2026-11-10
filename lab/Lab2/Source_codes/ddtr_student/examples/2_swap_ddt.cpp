#include <ddtr_library>
#include <iostream>

// The same as 1 on any DDT, picked when building:
//   g++ ... -Dsel_dt_scores=red_black_tree examples/2_swap_ddt.cpp

#ifndef sel_dt_scores
#define sel_dt_scores vector
#endif
using Scores = ddtr::sel_dt_scores<int>;

int main() {
    Scores scores;
    for (int s : {72, 95, 61, 88, 79, 54, 90})
        scores.push_back(s);
    scores.sort();

    std::cout << "lowest " << scores.read(0) << ", median " << scores.read(scores.size() / 2)
              << ", highest " << scores.read(scores.size() - 1) << '\n';

    scores.remove_value(54);
    std::cout << "without 54 the lowest is " << scores.read(0) << '\n';
}
