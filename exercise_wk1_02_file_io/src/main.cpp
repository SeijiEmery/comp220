// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// File I/O exercise: writes a CSV (excel-compatible) file with some data.
//
// Note: noticed that the XLS format described is actually CSV (which can ommit commas).
// XLS is actually a binary format, and my version of excel was not happy trying
// to load this; CSV was fine (and quite standard) so I used that.
//

#include <fstream>

double approx_pi    (unsigned);
double approx_sqrt  (double, unsigned);

// Helper function: write variadic args to an ostream
template <typename T>
void writeCsvRow (std::ostream& os, T item) {
    os << item << '\n';
}
template <typename T, typename... Args>
void writeCsvRow (std::ostream& os, T item, Args... args) {
    os << item << ',' << '\t';
    writeCsvRow(os, args...);
}

int main () {
    // Open file to write to via ofstream
    std::ofstream file { "output.csv", std::ofstream::out };

    // Write header
    writeCsvRow(file, "n", "sqrt(2)", "pi");
    
    // Write body (data for n iterations of sqrt, pi algorithms)
    for (auto i = 0; i <= 10; ++i) {
        writeCsvRow(file, i, approx_sqrt(2,i), approx_pi(i));
    }

    // file.close() called via RAII.
}

// My algorithm: https://gist.github.com/SeijiEmery/c6139d82ab7cbce8eeaef02e89358475
double approx_pi (unsigned n) {
    double x = 1;
    for (auto i = n; i --> 0; ) {
        x = 2 - approx_sqrt(4 - x, 20);
    }
    return 3 * (1 << n) * approx_sqrt(x, 20);
}

// Newton's method
double approx_sqrt (double S, unsigned n) {
    double x = 1;
    while (n --> 0) {
        x -= (x * x - S) / (2 * x);
    }
    return x;
}
