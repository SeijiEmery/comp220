// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Array.TestDriver.cpp
// Tests our dynamic array implementation.
//
// Test implementation: uses custom test macros instead of assert().
// Uses global variables (bad but KISS) so obviously not threadsafe.
//
// ============================================================================
// Note: uses ANSI escape sequences to display colored text for test results.
// If you see funky stuff like "\033[32mAll tests passed\n\033[0m", run the program in a
// terminal / console that supports this feature: https://en.wikipedia.org/wiki/ANSI_escape_code
//
// Xcode does not, but you can run it properly with the following steps:
//  * click Products folder > right click <target executable>
//  * click show in finder
//  * double click on <target executable> to run in terminal
//
// I'm not sure whether this will work on windows (it should in some terminals but might not 
// work in CMD.exe (haven't tested); visual studio may be in the same situation as xcode)
// ============================================================================
//
// Remote source:
// https://github.com/SeijiEmery/comp220/tree/master/assignment_03/src/Array.TestDriver.cpp
//

#include <iostream>     // cerr, cout
#include <string>       // string
using namespace std;

#include "Array.h"
#include "Array.h" // multiple include test

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

struct TestInfo {
    unsigned passed = 0, failed = 0;
} testcase;

#define ASSERT_EQ(a,b) do {\
    if (a == b) ++testcase.passed, std::cout << "\033[32mPASS\033[0m: " #a " == " #b " (file " __FILE__ ":" << __LINE__ << ")\n";\
    else        ++testcase.failed, std::cout << "\033[31mFAIL\033[0m: " #a " == " #b " (file " __FILE__ ":" << __LINE__ << ")\n";\
    std::cout << "    EXPECTED: " #a " = '" << a << "'\n";\
    std::cout << "    GOT:      " #b " = '" << b << "'\n";\
} while(0)

#define ASSERT_NE(a,b) do {\
    if (a != b) ++testcase.passed, std::cout << "\033[32mPASS\033[0m: " #a " != " #b " (file " __FILE__ ":" << __LINE__ << ")\n";\
    else        ++testcase.failed, std::cout << "\033[31mFAIL\033[0m: " #a " != " #b " (file " __FILE__ ":" << __LINE__ << ")\n";\
    std::cout << "    EXPECTED: " #a " = '" << a << "'\n";\
    std::cout << "    GOT:      " #b " = '" << b << "'\n";\
} while(0)

#define SECTION(msg...) if ((std::cout << "\n\033[33m" << msg << "\033[0m\n"), true)

static void reportTestResults () {
    std::cout << (testcase.failed ? "\033[31m" : "\033[32m") << testcase.passed << " / " << (testcase.passed + testcase.failed) << " tests passed\033[0m\n\n";
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
        Array<T> array;

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
            const Array<T> array2 = array;
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
