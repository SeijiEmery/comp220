// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// AssociateArray.TestDriver.cpp
//

#include <iostream>     // cerr, cout
#include <string>       // string
#include <cstring>
using namespace std;

#include "AssociativeArray.h"
#include "AssociativeArray.h" // multiple include test


template <typename AA, typename Key, typename Value>
void _testAAImpl (const char*, const Key[], const Value[]);

#define TEST_AA_IMPL(AA, S, K, V, keys, values) \
    _testAAImpl<AA<K,V,S>>(#AA "<" #K ", " #V ", " #S ">", keys, values)

template <typename K, typename V>
std::ostream& operator<< (std::ostream& os, const std::pair<K, V> pair) {
    return os << "{ " << pair.first << ", " << " }";
}


int main () {
    std::cout << "Programmer: Seiji Emery\n";
    std::cout << "Programmer ID: M00202623\n";
    std::cout << "File: " __FILE__ "\n";

    const int ints[4]    = { 1, 2, 3, 4 };
    const double doubles[4] = { -1.1, 0.0, 3.14159, 22.9 };
    const char chars[4]   = { '@', 'Z', 'a', '$' };
    const std::string strings[4] = { "foo", "bar", "baz", "borg" };
    typedef std::pair<std::string, int> pair;
    const pair pairs[4] = { { "lorp", 1 }, { "torg", 2 }, { "mal", -1 }, { "b", 10 } };

    #define TEST_WITH_KEYS(AA, S, K, keys) \
        TEST_AA_IMPL(AA, S, K, int, keys, ints); \
        TEST_AA_IMPL(AA, S, K, double, keys, doubles); \
        TEST_AA_IMPL(AA, S, K, char, keys, chars); \
        TEST_AA_IMPL(AA, S, K, std::string, keys, strings); \
        TEST_AA_IMPL(AA, S, K, pair, keys, pairs);

    #define TEST_WITH_STRATEGY(AA, Strategy) \
        TEST_WITH_KEYS(AA, Strategy, int, ints) \
        TEST_WITH_KEYS(AA, Strategy, double, doubles) \
        TEST_WITH_KEYS(AA, Strategy, char, chars) \
        TEST_WITH_KEYS(AA, Strategy, std::string, strings) \
        TEST_WITH_KEYS(AA, Strategy, pair, pairs)

    TEST_WITH_STRATEGY(AssociativeArray, AADefaultStrategy)
    TEST_WITH_STRATEGY(AssociativeArray, AAFastBinaryStrategy)
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
    mintest::writeln() << "    EXPECTED: " #b " = '" << (b) << "'";\
    mintest::writeln() << "    GOT:      " #a " = '" << (a) << "'";\
} while(0)
#define ASSERT_EQ(a,b) ASSERT_BIN_OP(a,b,==)
#define ASSERT_NE(a,b) ASSERT_BIN_OP(a,b,!=)
#define SECTION(msg...) if (auto testcase = mintest::TestInfo((\
    mintest::writeln() << SET_YELLOW << msg << CLEAR_COLOR, true)))





template <typename AA, typename Key, typename Value>
void _testAAImpl (
    const char* name,
    const Key keys[],
    const Value values[]
) {
    SECTION("Testing " << name) {
        AA dict;
        SECTION("Should initially be empty") {
            ASSERT_EQ(dict.size(), 0);
            ASSERT_EQ(bool(dict), false);
        }
        SECTION("Test insertion") {
            dict[keys[0]] = values[0];
            ASSERT_EQ(dict[keys[0]], values[0]);
            ASSERT_EQ(dict.size(), 1);
            ASSERT_EQ(bool(dict), true);

            dict[keys[1]] = values[1];
            ASSERT_EQ(dict[keys[1]], values[1]);
            ASSERT_EQ(dict.size(), 2);
        }
        SECTION("Test duplicate insertion") {
            dict[keys[0]] = values[1];
            ASSERT_EQ(dict[keys[0]], values[1]);
            ASSERT_EQ(dict.size(), 2);
        }
        SECTION("Test key checking") {
            ASSERT_EQ(dict.containsKey(keys[0]), true);
            ASSERT_EQ(dict.containsKey(keys[0]), true);
            ASSERT_EQ(dict.containsKey(keys[1]), true);
            ASSERT_EQ(dict.containsKey(keys[2]), false);
            ASSERT_EQ(dict.containsKey(keys[2]), false);
            ASSERT_EQ(dict.size(), 2);
        }
        SECTION("Test key removal") {
            dict.deleteKey(keys[0]);
            ASSERT_EQ(dict.size(), 1);
            ASSERT_EQ(dict.containsKey(keys[0]), false);
        }
        SECTION("Test removal of nonexistant key") {
            dict.deleteKey(keys[0]);
            ASSERT_EQ(dict.size(), 1);
            ASSERT_EQ(dict.containsKey(keys[0]), false);
        }
        SECTION("Test many-insert") {
            dict.insert({
                { keys[0], values[1] },
                { keys[0], values[0] },
                { keys[1], values[2] },
                { keys[2], values[2] },
                { keys[3], values[3] },
            });
            ASSERT_EQ(dict.size(), 4);
            ASSERT_EQ(dict[keys[0]], values[0]);
            ASSERT_EQ(dict[keys[1]], values[2]);
            ASSERT_EQ(dict[keys[2]], values[2]);
            ASSERT_EQ(dict[keys[3]], values[3]);
        }
        SECTION("Test copy-construction") {
            AA second { dict };
            ASSERT_EQ(second.size(), 4);
            ASSERT_EQ(second[keys[0]], values[0]);
            ASSERT_EQ(second[keys[1]], values[2]);
            ASSERT_EQ(second[keys[2]], values[2]);
            ASSERT_EQ(second[keys[3]], values[3]);

            SECTION("Copy modification should not affect original") {
                second[keys[0]] = values[3];
                ASSERT_EQ(second[keys[0]], values[3]);
                ASSERT_EQ(dict[keys[0]], values[0]);

                second.deleteKey(keys[0]);
                second.deleteKey(keys[1]);
                ASSERT_EQ(second.size(), 2);
                ASSERT_EQ(dict.size(), 4);
            }
        }
        SECTION("Test assignment") {
            AA second;
            ASSERT_EQ(second.size(), 0);

            second = dict;
            ASSERT_EQ(second.size(), 4);
            ASSERT_EQ(second[keys[0]], values[0]);
            ASSERT_EQ(second[keys[1]], values[2]);
            ASSERT_EQ(second[keys[2]], values[2]);
            ASSERT_EQ(second[keys[3]], values[3]);

            SECTION("Copy modification should not affect original") {
                second[keys[0]] = values[3];
                ASSERT_EQ(second[keys[0]], values[3]);
                ASSERT_EQ(dict[keys[0]], values[0]);

                second.deleteKey(keys[0]);
                second.deleteKey(keys[1]);
                ASSERT_EQ(second.size(), 2);
                ASSERT_EQ(dict.size(), 4);
            }
        }
        SECTION("Test clear()") {
            dict.clear();
            ASSERT_EQ(dict.size(), 0);
            ASSERT_EQ(dict.containsKey(keys[0]), false);
            ASSERT_EQ(dict[keys[0]], Value());
            dict.clear();
        }
        SECTION("hasKey() on empty value should not insert") {
            ASSERT_EQ(dict.containsKey(keys[0]), false);
            ASSERT_EQ(dict.size(), 0);
        }
        SECTION("const operator[] on empty value should not insert") {
            const AA& constref = dict;
            ASSERT_EQ(constref[keys[0]], Value());
            ASSERT_EQ(constref.size(), 0);
        }
        SECTION("non-const operator[] should insert") {
            ASSERT_EQ(dict[keys[0]], Value());
            ASSERT_EQ(dict.size(), 1);
        }
    }
}
