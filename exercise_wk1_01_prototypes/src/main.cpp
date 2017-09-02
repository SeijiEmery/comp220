// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Code Alignment and Organization, Exercise 2.
//
// Demonstrates correct usage of function prototypes.
//

#include <cstdio>   // printf
#include <cmath>    // sqrt

void print_pi (unsigned);

int main () {
    print_pi(10);
}

double approx_pi (unsigned iterations) {
    double x = 1;
    for (auto n = iterations; n --> 0; ) {
        x = 2 - sqrt(4 - x);
    }
    return 3 * (1 << iterations) * sqrt(x);
}
void print_pi (unsigned iterations) {
    printf("π ≈ %lf\n", approx_pi(10));
}
