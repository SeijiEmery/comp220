#include <iostream>
#include <cstdlib>
#include "SortableArray.h"

int main () {
    SortableArray<int> array;
    srand(time(nullptr));
    for (size_t n = 0; n < 30; ++n) {
        std::cout << SET_CYAN << "[ ";
        for (size_t i = 0; i < n; ++i) {
            array[i] = (rand() % 90 + 10);
            std::cout << array[i] << " ";
        }
        std::cout << "]\n";

        array.sort(n);
        std::cout << "[ ";
        for (size_t i = 0; i < n; ++i) {
            std::cout << (array[i] < array[i+1] ? CLEAR_COLOR : SET_RED);
            std::cout << array[i] << " ";
        }
        std::cout << CLEAR_COLOR "]\n";
    }
}
