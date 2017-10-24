// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Part3.cpp
//
// Implements + runs a simple bubblesort to illustrate the performance of O(n^2) algorithms.
//
// remote source: https://github.com/SeijiEmery/comp220/blob/master/assignment_08/src/Part3.cpp
//

#include <algorithm>
#include <iostream>
#include <iomanip>

using namespace std;
#include <cassert>

template <typename Iterator>
void bubbleSort (Iterator begin, Iterator end) {
    for (; begin != end; ++begin) {
        for (auto other = begin; ++other != end; ) {
            if (*begin > *other) {
                std::swap(*begin, *other);
            }
        }
    }
}

template <typename Iterator>
bool sorted (Iterator begin, Iterator end) {
    if (!begin || begin == end) return true;
    for (auto next = begin; ++next != end; begin = next) {
        if (*begin > *next)
            return false;
    }
    return true;
}
static void unittest_sorted () {
    { int s[] {};           assert(sorted(&s[0], &s[sizeof(s) / sizeof(*s)]) == true);  }
    { int s[] { 1 };        assert(sorted(&s[0], &s[sizeof(s) / sizeof(*s)]) == true);  }
    { int s[] { 1, 2 };     assert(sorted(&s[0], &s[sizeof(s) / sizeof(*s)]) == true);  }
    { int s[] { 1, 1 };     assert(sorted(&s[0], &s[sizeof(s) / sizeof(*s)]) == true);  }
    { int s[] { 2, 1 };     assert(sorted(&s[0], &s[sizeof(s) / sizeof(*s)]) == false); }
    { int s[] { 1, 3, 2 };  assert(sorted(&s[0], &s[sizeof(s) / sizeof(*s)]) == false); }
    { int s[] { 1, 2, 3 };  assert(sorted(&s[0], &s[sizeof(s) / sizeof(*s)]) == true);  }
}


template <typename T> T randUniform () {
    return static_cast<T>(rand()) / RAND_MAX;
}
template <typename T> T randRange (T min, T max) {
    return randUniform<T>() * (max - min) + min;
}
template <typename T, typename Iterator>
void randomize (Iterator begin, Iterator end, T min, T max) {
    for (; begin != end; ++begin) {
        *begin = randRange(min, max);
    }
}

template <typename Iterator>
void show (Iterator begin, Iterator end) {
    std::cout << "{  ";
    for (; begin != end; ++begin) {
        std::cout << *begin << ", ";
    }
    std::cout << "\b\b}\n";    
}

template <typename F, typename... Args>
double benchmark (size_t iterations, F fcn, Args... args) {
    clock_t startTime = clock();
    for (size_t i = iterations; i --> 0; ) {
        fcn(args...);
    }
    clock_t endTime = clock();
    return static_cast<double>(endTime - startTime) / CLOCKS_PER_SEC * 1e3 / iterations;
}


int main () {
    std::cout << "Programmer: Seiji Emery\n"
              << "Programmer's id: M00202623\n"
              << "File: " __FILE__ "\n\n";
    unittest_sorted();
    srand(time(nullptr));

    std::cout << "Bubble sort runtimes (quadratic):\n";

    double expected = 0.0;
    for (auto n = 64, k = 10; k --> 0; n *= 2) {
        double* array = new double[n];
        randomize(&array[0], &array[n], 0.0, 1.0);
        auto runtime = benchmark(1, [&](){ bubbleSort(&array[0], &array[n]); });
        assert(sorted(&array[0], &array[n]));
        //show(&array[0], &array[n]);
        delete[] array;

        std::cout << "sorted " << std::setw(6) << n << " items in "
            << std::setw(8) << runtime << " ms / run  expected ";
        if (expected == 0) std::cout << "O(n^2)\n";
        else               std::cout << expected << '\n';

        expected = runtime * 2;
    }
    return 0;
}
