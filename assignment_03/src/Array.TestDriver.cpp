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
// To disable unittest indentation, compile with -D NO_TEST_INDENT
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

namespace mintest {

// global variables (note: NOT threadsafe / reentrant)
struct TestInfo;
static TestInfo* g_currentTest = nullptr;
static int g_testIndent = 0;

// Helper functions for printing + controlling indented, formatted lines
static std::ostream& writeln () { 
    #ifndef NO_TEST_INDENT
        std::cout << "\n" SET_YELLOW;
        for (auto i = g_testIndent; i --> 0; ) {
            std::cout << "|  ";      // prints '|' character every x spaces to denote indentation level
        }  
        return std::cout << CLEAR_COLOR; 
    #else
        return std::cout << '\n';
    #endif
}
static void indent () { ++g_testIndent; }
static void dedent () { --g_testIndent; }

// Stores unittest section info; linked list to previous test sections.
struct TestInfo {
    unsigned passed = 0, failed = 0;
    TestInfo* prev = nullptr;

    // Indent when section entered; output done via macro + comma operator
    // since handing <<-ed args would be difficult. 
    TestInfo (bool) : prev(g_currentTest) { g_currentTest = this; indent(); }

    ~TestInfo () {
        // Dedent, and write section test results when section block ends
        dedent();        
        if (passed || failed) {
            writeln() << (failed ? SET_RED : SET_GREEN) 
                << passed << " / " << (passed + failed) << " tests passed" CLEAR_COLOR;
            writeln();
        }
        if ((g_currentTest = prev)) {
            // If not last section, add test results to previous test section.
            prev->failed += failed;
            prev->passed += passed;
        } else if (failed) {
            // Otherwise, call exit() if any of the previous tests have failed.
            exit(-1);   
        }
    }
    // Always evaluate to true, used since we're declaring sections in an if statement
    operator bool () { return true; }
}; // struct TestInfo
}; // namespace mintest

// ASSERT + SECTION macros.
// SECTION(message << args...): declares a new scoped section (w/ a label etc.; can be formatted via << and macro magic).
// ASSERT_EQ(a, b):     pretty-printed version of assert(a == b)
// ASSERT_NE(a, b):     pretty-printed version of assert(a != b)

#define ASSERT_BIN_OP(a,b,op) do {\
    mintest::writeln() << ((a) op (b) ? \
        (++testcase.passed, SET_GREEN "PASS") : \
        (++testcase.failed, SET_RED   "FAIL")) \
        << CLEAR_COLOR ": " #a " " #op " " #b " (file " __FILE__ ":" << __LINE__ << ")";\
    mintest::writeln() << "    EXPECTED: " #b " = '" << b << "'";\
    mintest::writeln() << "    GOT:      " #a " = '" << a << "'";\
} while(0)
#define ASSERT_EQ(a,b) ASSERT_BIN_OP(a,b,==)
#define ASSERT_NE(a,b) ASSERT_BIN_OP(a,b,!=)
#define SECTION(msg...) if (auto testcase = mintest::TestInfo((\
    mintest::writeln() << SET_YELLOW << msg << CLEAR_COLOR, true)))

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
        Array<T> array (100);

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
            array.capacity(N);
            auto* ptr = &(array[-1] = first);

            ASSERT_EQ(*ptr, first);
            ASSERT_EQ(array[-1], init);
            // ASSERT_EQ(array[N], array[-1]);
            // ASSERT_EQ(array[N], init);
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

        SECTION("Object copy test") {
            Array<T> array2 (array);
            ASSERT_EQ(array2[0], first);
            ASSERT_EQ(array[13], second);
            ASSERT_EQ(array[N-1], third);

            int numNonEqualElements = 0;            
            for (auto i = 0; i < array.capacity(); ++i) {
                if (array[i] != array2[i]) ++numNonEqualElements;
            }
            ASSERT_EQ(numNonEqualElements, 0);
        }

        SECTION("Object assignment test") {
            Array<T> array2; array2 = array;
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
