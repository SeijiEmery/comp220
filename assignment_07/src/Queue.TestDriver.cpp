// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Queue.TestDriver.cpp
//
// Remote source:
// https://github.com/SeijiEmery/comp220/tree/master/assignment_07/src/Queue.TestDriver.cpp
//

#include <iostream>     // cerr, cout
#include <string>       // string
#include <cstring>
using namespace std;

#include "Queue.h"
#include "Queue.h" // multiple include test

template <typename Queue, typename T>
void _testQueueImpl (const char*, const char*, T, T, T, T);

#define TEST_QUEUE_IMPL(Queue, T, first, second, third) \
    _testQueueImpl<Queue<T>,T>(#Queue, #T, {}, first, second, third)

int main () {
    std::cout << "Programmer: Seiji Emery\n";
    std::cout << "Programmer ID: M00202623\n";
    std::cout << "File: " __FILE__ "\n";

    #define TEST_QUEUE_IMPL_WITH(Queue) \
        TEST_QUEUE_IMPL(Queue, int, 1, 2, 3); \
        TEST_QUEUE_IMPL(Queue, double, 1.5, 2.5, 3.5); \
        TEST_QUEUE_IMPL(Queue, char, '@', 'Z', 'a'); \
        TEST_QUEUE_IMPL(Queue, std::string, "bar", "baz", "foo");

    TEST_QUEUE_IMPL_WITH(Queue)
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
template <typename Queue, typename T>
void _testQueueImpl (const char* queueName, const char* typeName, T init, T first, T second, T third) {
    SECTION("Testing " << queueName << "<" << typeName << ">") {
        SECTION("Sanity check") {
            // ASSERT_NE(first, first);     // To verify that test framework is working, try uncommenting this line (should fail).
            ASSERT_EQ(first, first);
            ASSERT_NE(first, second);
        }
        Queue queue;

        SECTION("Queue should initially be empty") {
            ASSERT_EQ(queue.size(), 0);
            ASSERT_EQ(queue.empty(), true);
        }
        SECTION("Test pushing first element") {
            queue.push(first);
            ASSERT_EQ(queue.size(), 1);
            ASSERT_EQ(queue.empty(), false);
            ASSERT_EQ(queue.peek(), first);
        }
        SECTION("Test pushing second element") {
            queue.push(second);
            ASSERT_EQ(queue.size(), 2);
            ASSERT_EQ(queue.empty(), false);
            ASSERT_EQ(queue.peek(), second);
        }
        SECTION("Const object test / test copy construction") {
            const Queue s2 = queue;
            ASSERT_EQ(s2.size(), 2);
            ASSERT_EQ(s2.empty(), false);
            ASSERT_EQ(s2.peek(), second);
            ASSERT_EQ(s2.size(), 2);
        }
        SECTION("Test popping elements") {
            queue.pop();
            ASSERT_EQ(queue.size(), 1);
            ASSERT_EQ(queue.empty(), false);
            ASSERT_EQ(queue.peek(), first);

            queue.push(second);
            queue.push(first);
            queue.push(third);
            ASSERT_EQ(queue.size(), 4);
            ASSERT_EQ(queue.empty(), false);
            ASSERT_EQ(queue.peek(), third);
            queue.pop(); ASSERT_EQ(queue.size(), 3); ASSERT_EQ(queue.peek(), first);
            queue.pop(); ASSERT_EQ(queue.size(), 2); ASSERT_EQ(queue.peek(), second);
            queue.pop(); ASSERT_EQ(queue.size(), 1); ASSERT_EQ(queue.peek(), first);
            queue.pop(); ASSERT_EQ(queue.size(), 0); ASSERT_EQ(queue.empty(), true);
            queue.pop(); ASSERT_EQ(queue.size(), 0); ASSERT_EQ(queue.empty(), true);
            queue.push(first);
            ASSERT_EQ(queue.size(), 1); ASSERT_EQ(queue.empty(), false);
            queue.pop();
            ASSERT_EQ(queue.empty(), true);
        }
        SECTION("Test clear") {
            queue.push(first);
            queue.push(second);
            queue.push(third);
            ASSERT_EQ(queue.size(), 3);
            queue.clear();
            ASSERT_EQ(queue.size(), 0); ASSERT_EQ(queue.empty(), true);
            queue.clear();
            ASSERT_EQ(queue.size(), 0); ASSERT_EQ(queue.empty(), true);
        }
    }
}
