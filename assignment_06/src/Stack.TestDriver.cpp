// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Stack.TestDriver.cpp
//

#include <iostream>     // cerr, cout
#include <string>       // string
#include <cstring>
using namespace std;

#include "Stack.h"
#include "Stack.h" // multiple include test

template <typename Stack, typename T>
void _testStackImpl (const char*, const char*, T, T, T, T);

#define TEST_STACK_IMPL(Stack, T, first, second, third) \
    _testStackImpl<Stack<T>,T>(#Stack, #T, {}, first, second, third)

int main () {
    std::cout << "Programmer: Seiji Emery\n";
    std::cout << "Programmer ID: M00202623\n";
    std::cout << "File: " __FILE__ "\n";

    #define TEST_STACK_IMPL_WITH(Stack) \
        TEST_STACK_IMPL(Stack, int, 1, 2, 3); \
        TEST_STACK_IMPL(Stack, double, 1.5, 2.5, 3.5); \
        TEST_STACK_IMPL(Stack, char, '@', 'Z', 'a'); \
        TEST_STACK_IMPL(Stack, std::string, "bar", "baz", "foo");

    TEST_STACK_IMPL_WITH(Stack)
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


//
// Test implementation
//
template <typename Stack, typename T>
void _testStackImpl (const char* stackName, const char* typeName, T init, T first, T second, T third) {
    SECTION("Testing " << stackName << "<" << typeName << ">") {
        SECTION("Sanity check") {
            // ASSERT_NE(first, first);     // To verify that test framework is working, try uncommenting this line (should fail).
            ASSERT_EQ(first, first);
            ASSERT_NE(first, second);
        }
        Stack stack;

        SECTION("Stack should initially be empty") {
            ASSERT_EQ(stack.size(), 0);
            ASSERT_EQ(stack.empty(), true);
        }
        SECTION("Test pushing first element") {
            stack.push(first);
            ASSERT_EQ(stack.size(), 1);
            ASSERT_EQ(stack.empty(), false);
            ASSERT_EQ(stack.peek(), first);
        }
        SECTION("Test pushing second element") {
            stack.push(second);
            ASSERT_EQ(stack.size(), 2);
            ASSERT_EQ(stack.empty(), false);
            ASSERT_EQ(stack.peek(), second);
        }
        SECTION("Const object test / test copy construction") {
            const Stack s2 = stack;
            ASSERT_EQ(s2.size(), 2);
            ASSERT_EQ(s2.empty(), false);
            ASSERT_EQ(s2.peek(), second);
            ASSERT_EQ(s2.size(), 2);
        }
        SECTION("Test popping elements") {
            stack.pop();
            ASSERT_EQ(stack.size(), 1);
            ASSERT_EQ(stack.empty(), false);
            ASSERT_EQ(stack.peek(), first);

            stack.push(second);
            stack.push(first);
            stack.push(third);
            ASSERT_EQ(stack.size(), 4);
            ASSERT_EQ(stack.empty(), false);
            ASSERT_EQ(stack.peek(), third);
            stack.pop(); ASSERT_EQ(stack.size(), 3); ASSERT_EQ(stack.peek(), first);
            stack.pop(); ASSERT_EQ(stack.size(), 2); ASSERT_EQ(stack.peek(), second);
            stack.pop(); ASSERT_EQ(stack.size(), 1); ASSERT_EQ(stack.peek(), first);
            stack.pop(); ASSERT_EQ(stack.size(), 0); ASSERT_EQ(stack.empty(), true);
            stack.pop(); ASSERT_EQ(stack.size(), 0); ASSERT_EQ(stack.empty(), true);
            stack.push(first);
            ASSERT_EQ(stack.size(), 1); ASSERT_EQ(stack.empty(), false);
            stack.pop();
            ASSERT_EQ(stack.empty(), true);
        }
        SECTION("Test clear") {
            stack.push(first);
            stack.push(second);
            stack.push(third);
            ASSERT_EQ(stack.size(), 3);
            stack.clear();
            ASSERT_EQ(stack.size(), 0); ASSERT_EQ(stack.empty(), true);
            stack.clear();
            ASSERT_EQ(stack.size(), 0); ASSERT_EQ(stack.empty(), true);
        }
    }
}
