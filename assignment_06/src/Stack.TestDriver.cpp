// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Stack.TestDriver.cpp
//

#include <iostream>     // cerr, cout
#include <string>       // string
#include <cstring>
using namespace std;

#include "StaticArray.h"
#include "StaticArray.h" // multiple include test

template <typename T, size_t N>
void _testArrayImpl (const char*, T, T, T, T);

#define TEST_ARRAY_IMPL(T, first, second, third) \
    _testArrayImpl<T,100>(#T, {}, first, second, third)

int main () {
    std::cout << "Programmer: Seiji Emery\n";
    std::cout << "Programmer ID: M00202623\n";
    std::cout << "File: " __FILE__ "\n";

    TEST_ARRAY_IMPL(int, 1, 2, 3);
    TEST_ARRAY_IMPL(double, 1.5, 2.5, 3.5);
    TEST_ARRAY_IMPL(char, '@', 'Z', 'a');
    TEST_ARRAY_IMPL(std::string, "bar", "baz", "foo");
    std::cout << "\033[32mAll tests passed\n\033[0m";
    return 0;
}

//
// Minimalistic test 'framework' for comp220. Extremely simple, etc.
//

#ifndef NO_ANSI_COLORS
    #define SET_COLOR(code) "\033[" #code "m"
#else
    #define SET_COLOR(code) ""
#endif
#define CLEAR_COLOR SET_COLOR(0)
#define SET_RED     SET_COLOR(31)
#define SET_GREEN   SET_COLOR(32)
#define SET_YELLOW  SET_COLOR(33)

struct TestInfo {
    unsigned passed = 0, failed = 0;
} testcase;

#define ASSERT_BIN_OP(a,b,op) do {\
    if ((a) op (b)) ++testcase.passed, std::cout << SET_GREEN "PASS" CLEAR_COLOR ": "; \
    else            ++testcase.failed, std::cout << SET_RED   "FAIL" CLEAR_COLOR ": "; \
    std::cout << #a " " #op " " #b " (file " __FILE__ ":" << __LINE__ << ")\n"; \
    std::cout << "    EXPECTED: " #a " = '" << a << "'\n";\
    std::cout << "    GOT:      " #b " = '" << b << "'\n";\
} while(0)
#define ASSERT_EQ(a,b) ASSERT_BIN_OP(a,b,==)
#define ASSERT_NE(a,b) ASSERT_BIN_OP(a,b,!=)
#define SECTION(msg...) if ((std::cout << "\n" SET_YELLOW << msg << CLEAR_COLOR "\n"), true)

static void reportTestResults () {
std::cout << (testcase.failed ? SET_RED : SET_GREEN) 
    << testcase.passed << " / " << (testcase.passed + testcase.failed) 
    << " tests passed" CLEAR_COLOR "\n\n";
if (testcase.failed != 0) {
    exit(-1);
}
testcase.failed = testcase.passed = 0;
}


//
// Test implementation
//
template <typename T, size_t N>
void _testArrayImpl (const char* name, T init, T first, T second, T third) {
    SECTION("Testing StaticArray<" << name << ", " << N << ">") {
        SECTION("Sanity check") {
            // ASSERT_NE(first, first);     // To verify that test framework is working, try uncommenting this line (should fail).
            ASSERT_EQ(first, first);
            ASSERT_NE(first, second);   
        }
        StaticArray<T,N> array;

        SECTION("Testing StaticArray capacity (should equal " << N << ")") {
            ASSERT_EQ(array.capacity(), N);
        }
        SECTION("Testing StaticArray initial values (should be default-initialized, equal '" << init << "')") {
            int numNonEqualElements = 0;
            for (auto i = 0; i < array.capacity(); ++i) {
                if (array[i] != init) ++numNonEqualElements;
            }
            ASSERT_EQ(numNonEqualElements, 0);
        }
        SECTION("Testing StaticArray getter / setter") {
            array[0] = first;
            ASSERT_EQ(array[0], first);

            array[13] = second;
            ASSERT_EQ(array[13], second);

            array[N-1] = third;
            ASSERT_EQ(array[N-1], third);
        }
        SECTION("Testing out-of-bounds array values") {
            auto* ptr = &(array[-1] = first);

            ASSERT_EQ(*ptr, first);
            ASSERT_EQ(array[-1], init);
            ASSERT_EQ(&(array[N]), &(array[-1]));
            ASSERT_EQ(array[N], array[-1]);
            ASSERT_EQ(array[N], init);

            // REQUIRE_EQ(array[-1], first);
            // REQUIRE_EQ(array[N],  first);
            ASSERT_NE(array[N], array[N-1]);
        }

        SECTION("Const-object test") {
            const StaticArray<T, N> array2 = array;
            ASSERT_EQ(array2[0], first);
            ASSERT_EQ(array[13], second);
            ASSERT_EQ(array[N-1], third);

            int numNonEqualElements = 0;            
            for (auto i = 0; i < array.capacity(); ++i) {
                if (array[i] != array2[i]) ++numNonEqualElements;
            }
            ASSERT_EQ(numNonEqualElements, 0);
        }
    }
    reportTestResults();
}
