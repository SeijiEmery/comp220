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
//
// To disable text colors, compile with -D NO_ANSI_COLORS
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

static int g_testIndent = 0;
static std::ostream& writeln () { 
    std::cout << "\n" SET_YELLOW;
    for (auto i = g_testIndent; i --> 0; ) 
        std::cout << SET_YELLOW "|  ";
    return std::cout << CLEAR_COLOR; 
}

struct TestInfo;
static TestInfo* g_currentTest = nullptr;

struct TestInfo {
    unsigned passed = 0, failed = 0;
    TestInfo* prev = nullptr;

    TestInfo (bool) { prev = g_currentTest; g_currentTest = this; ++g_testIndent; }
    ~TestInfo () {
        --g_testIndent;        
        if (passed || failed) {
            writeln() << (failed != 0 ? SET_RED : SET_GREEN) 
                << passed << " / " << (passed + failed) 
                << " tests passed" CLEAR_COLOR;
            writeln();
        }
        if ((g_currentTest = prev)) {
            if (failed) ++prev->failed;
            else        ++prev->passed;
        } else if (failed) {
            exit(-1);
        }
    }
    operator bool () { return true; }
};

#define ASSERT_BIN_OP(a,b,op) do {\
    writeln() << ((a) op (b) ? \
        (++testcase.passed, SET_GREEN "PASS") : \
        (++testcase.failed, SET_RED   "FAIL")) \
        << CLEAR_COLOR ": " #a " " #op " " #b " (file " __FILE__ ":" << __LINE__ << ")";\
    writeln() << "    EXPECTED: " #a " = '" << a << "'";\
    writeln() << "    GOT:      " #b " = '" << b << "'";\
} while(0)
#define ASSERT_EQ(a,b) ASSERT_BIN_OP(a,b,==)
#define ASSERT_NE(a,b) ASSERT_BIN_OP(a,b,!=)
#define SECTION(msg...) if (auto testcase = TestInfo((writeln() << SET_YELLOW << msg << CLEAR_COLOR, true)))

//
// Main program
//

template <typename T, size_t N>
void _testArrayImpl (const char*, T, T, T, T);

#define TEST_ARRAY_IMPL(T, first, second, third) \
    _testArrayImpl<T,100>(#T, {}, first, second, third)

int main () {
    std::cout << "Programmer: Seiji Emery\n";
    std::cout << "Programmer ID: M00202623\n";
    std::cout << "File: " __FILE__ "\n";

    SECTION("Running Tests") {
        TEST_ARRAY_IMPL(int, 1, 2, 3);
        TEST_ARRAY_IMPL(double, 1.5, 2.5, 3.5);
        TEST_ARRAY_IMPL(char, '@', 'Z', 'a');
        TEST_ARRAY_IMPL(std::string, "bar", "baz", "foo");
    }
    // std::cout << "\033[32mAll tests passed\n\033[0m";
    return 0;
}

//
// Test implementation
//
template <typename T, size_t N>
void _testArrayImpl (const char* name, T init, T first, T second, T third) {
    SECTION("Testing Array<" << name << ", " << N << ">") {
        SECTION("Sanity check") {
            // ASSERT_NE(first, first);     // To verify that test framework is working, try uncommenting this line (should fail).
            ASSERT_EQ(first, first);
            ASSERT_NE(first, second);   
        }
        Array<T> array;

        SECTION("Testing Array capacity (should equal " << N << ")") {
            ASSERT_EQ(array.capacity(), N);
        }
        SECTION("Testing Array initial values (should be default-initialized, equal '" << init << "')") {
            int numNonEqualElements = 0;
            for (auto i = 0; i < array.capacity(); ++i) {
                if (array[i] != init) ++numNonEqualElements;
            }
            ASSERT_EQ(numNonEqualElements, 0);
        }
        SECTION("Testing Array getter / setter") {
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
}