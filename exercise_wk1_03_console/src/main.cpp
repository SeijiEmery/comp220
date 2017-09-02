// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Console programming exercise:
// Presents a random math problem to the user, demonstrates correct usage of cin, etc.
//
// Uses a lot of C standard library stuff as I find that more convenient for this kind of exercise.
// Will not use vector, etc. when we are writing our own data structures.
//

#include <iostream>     // std::cin
#include <cstdlib>      // rand(), atoi()
#include <ctime>        // time()
#include <cstdio>       // printf
#include <cassert>      // assert
#include <vector>       // std::vector
#include <string>       // std::string
#include <functional>   // std::function
using namespace std;


int main () {
    // List of ops that we can select from randomly. Implemented as a list of pairs of 
    // (string, binary-function), which are used to print / evaluate the operation, 
    // respectively. Implemented using lambdas + a macro for conciseness. 
    // Writing this via eg. a switch statement would be more error prone.
    #define DEFN_OP(op) { #op, [](int a, int b) { return a op b; } }
    std::vector<std::pair<std::string, std::function<int(int,int)>>> ops {
        DEFN_OP(+), DEFN_OP(-), DEFN_OP(*), DEFN_OP(/)
    };
    #undef DEFN_OP

    #define op   (ops[i].first)     // sorry, this is evil but clearer and less error prone
    #define fcn  (ops[i].second)

    srand(time(nullptr));           // seed PRNG with current time (will NOT be seeded randomly otherwise...)
    int i = rand() % ops.size();    // select a random operation
    int a = rand() % 10;            // random range [0, 10]
    int b = rand() % 9 + 1;         // random range [1, 10]     (prevent divide by zero)

    if (op == "/") {                // for division, change the first operand to something that cleanly divides.
        a = b * (rand() % 10);
    }

    // Print query, ask user for input
    printf("What is %d %s %d? ", a, op.c_str(), b);
    std::string buffer;
    std::cin >> buffer;

    // Check the user's answer.
    int result = atoi(buffer.c_str());
    if (result == fcn(a, b)) {
        printf("Correct!\n");
    } else {
        printf("Wrong, answer is %d\n", fcn(a, b));
    }

    #undef op
    #undef fcn
}
