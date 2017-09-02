// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Week 1, a very simple static array implementation.
// 
// Includes proper unittesting (catch.hpp, from https://github.com/philsquared/Catch)
// compile with g++ -Wall -std=c++1z ../src/MyArray.TestDriver.cpp; ./a.out
//
// To disable catch.hpp and force fallback tests, compile with -D NO_CATCH.
//
// Expects to be compiled with TestDriver in the same directory as MyArray.hpp.
// Reuses impl b/c DRY.
//

#define RUN_TESTS
#ifndef NO_CATCH
    #define USE_CATCH
#endif
#define NO_MAIN
#include "MyArray.cpp"
