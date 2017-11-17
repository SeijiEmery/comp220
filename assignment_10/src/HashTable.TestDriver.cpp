// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// AssociateArray.TestDriver.cpp
//

#include <iostream>     // cerr, cout
#include <string>       // string
#include <cstring>
#include <utility>
using namespace std;

#include "HashTable.h"
#include "HashTable.h" // multiple include test

//
// Minimalistic test 'framework' for comp220. Extremely simple, etc.
//

#ifndef NO_ANSI_COLORS
    #define SET_COLOR(code) "\033[" code "m"
#else
    #define SET_COLOR(code) ""
#endif
#define CLEAR_COLOR SET_COLOR("0")
#define SET_CYAN    SET_COLOR("36;1")
#define SET_RED     SET_COLOR("31;1")
#define SET_GREEN   SET_COLOR("32;1")
#define SET_YELLOW  SET_COLOR("33;1")
#define SET_BLUE    SET_COLOR("34;1")
#define SET_PINK    SET_COLOR("35;1")

struct LineWriter {
    std::ostream& os;
    bool shouldClearColor;
    LineWriter (std::ostream& os, const char* startColor = nullptr) : 
        os(os), shouldClearColor(startColor != nullptr)
    {
        if (startColor) { os << startColor; }
    }
    template <typename T>
    LineWriter& operator<< (const T& other) {
        return os << other, *this;
    }
    ~LineWriter () {
        if (shouldClearColor) { os << CLEAR_COLOR "\n"; }
        else { os << '\n'; }
    }
};
LineWriter writeln  (std::ostream& os, const char* color = nullptr) { return LineWriter(os, color); }
LineWriter writeln  (const char* color = nullptr) { return LineWriter(std::cout, color); }
LineWriter report (std::ostream& os = std::cout) { return writeln(os, SET_CYAN); }
LineWriter warn   (std::ostream& os = std::cout) { return writeln(os, SET_RED); }
LineWriter info   (std::ostream& os = std::cout) { return writeln(os, SET_GREEN); }


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



template <typename HT, typename Key, typename Value>
void _testHTImpl (
    const char* name,
    HT          hashtable, 
    const Key   keys[], 
    const Value values[]
) {
    SECTION("Testing " << name << "::Storage") {
        auto storage = hashtable.make_storage(10);

        SECTION("test initial values") {
            ASSERT_EQ(storage.size(), 10);

            SECTION("by iterative inspection") {
                size_t numSetValues = 0;
                for (size_t i = storage.size(); i --> 0; ) {
                    if (storage.contains(i)) {
                        ++numSetValues;
                    }
                }
                ASSERT_EQ(numSetValues, 0);
            }
            SECTION("by each()") {
                size_t numSetValues = 0;
                storage.each([&](size_t i, bool set, const std::pair<Key,Value>& values) {
                    if (set) {
                        ++numSetValues;
                    }
                });
                ASSERT_EQ(numSetValues, 0);
            }
            SECTION("by iteration") {
                size_t numSetValues = 0;
                for (auto& kv : storage) {
                    ++numSetValues;
                } 
                ASSERT_EQ(numSetValues, 0);
            }
            SECTION("test iterators") {
                ASSERT_EQ(storage.begin(),  storage.begin());
                ASSERT_EQ(storage.begin(),  storage.end());
                ASSERT_EQ(storage.end(),    storage.end());

                ASSERT_EQ(storage.begin(),  storage.cbegin());
                ASSERT_EQ(storage.cbegin(), storage.begin());
                ASSERT_EQ(storage.end(),    storage.cend());
                ASSERT_EQ(storage.cend(),   storage.end());
            }
        }
        SECTION("test maybeInsert key") {
            ASSERT_EQ(storage.maybeInsert(3, keys[0]), true);
            ASSERT_EQ(storage.maybeInsert(3, keys[1]), false);
            ASSERT_EQ(storage.contains(3), true);
            ASSERT_EQ(storage[3].first, keys[0]);
            ASSERT_EQ(storage[3].second, Value{});

            SECTION("test iteration") {
                auto it = storage.begin();
                ASSERT_NE(it, storage.end());
                ASSERT_EQ(&(*it), &storage[3]);
                ASSERT_EQ((*it).first, keys[0]);
                ASSERT_EQ((*it).second, Value{});
                ASSERT_EQ(it->first, keys[0]);
                ASSERT_EQ(it->second, Value{});
                ++it;
                ASSERT_EQ(it, storage.end());
                ++it;
                ASSERT_EQ(it, storage.end());
            }
        }
        SECTION("test maybeInsert key-value") {
            ASSERT_EQ(storage.maybeInsert(4, std::pair<Key,Value> { keys[0], values[1] }), true);
            ASSERT_EQ(storage.maybeInsert(4, std::pair<Key,Value> { keys[1], values[0] }), false);
            ASSERT_EQ(storage.contains(4), true);
            ASSERT_EQ(storage[4].first, keys[0]);
            ASSERT_EQ(storage[4].second, values[1]);

            SECTION("test iteration") {
                auto it = storage.begin();
                ASSERT_NE(it, storage.end());
                ASSERT_EQ(&(*it), &storage[3]);
                ++it;
                ASSERT_NE(it, storage.end());
                ASSERT_EQ(&(*it), &storage[4]);
                ++it;
                ASSERT_EQ(it, storage.end());
                ++it;
                ASSERT_EQ(it, storage.end());
            }
        }
        SECTION("test no other values inserted") {
            size_t numSetValues = 0;
            storage.each([&](size_t i, bool set, const std::pair<Key,Value>& values) {
                if (set) {
                    ++numSetValues;
                }
            });
            ASSERT_EQ(numSetValues, 2);

            SECTION("test non-const iteration") {
                size_t i = 0;
                for (auto& value : storage) {
                    switch (i++) {
                        case 0: ASSERT_EQ(&value, &storage[3]); break;
                        case 1: ASSERT_EQ(&value, &storage[4]); break;
                        default: ASSERT_EQ(true, false);
                    }
                }
            }
            SECTION("test const iteration") {
                size_t i = 0;
                for (const auto& value : storage) {
                    switch (i++) {
                        case 0: ASSERT_EQ(&value, &storage[3]); break;
                        case 1: ASSERT_EQ(&value, &storage[4]); break;
                        default: ASSERT_EQ(true, false);
                    }
                }
            }
        }
        SECTION("test swap") {
            auto s2 = hashtable.make_storage(20);

            SECTION("setup second storage element") {
                ASSERT_EQ(s2.size(), 20);

                ASSERT_EQ(s2.maybeInsert(1, keys[4]), true);
                ASSERT_EQ(s2.contains(1), true);
                ASSERT_EQ(s2.contains(3), false);
                ASSERT_EQ(s2.contains(4), false);
            }
            SECTION("after swap") {
                storage.swap(s2);

                ASSERT_EQ(s2.size(), 10);
                ASSERT_EQ(s2.contains(1), false);
                ASSERT_EQ(s2.contains(3), true);
                ASSERT_EQ(s2.contains(4), true);

                ASSERT_EQ(storage.size(), 20);
                ASSERT_EQ(storage.contains(1), true);
                ASSERT_EQ(storage.contains(3), false);
                ASSERT_EQ(storage.contains(4), false);
            }
            SECTION("after second swap") {
                 storage.swap(s2);

                ASSERT_EQ(storage.size(), 10);
                ASSERT_EQ(storage.contains(1), false);
                ASSERT_EQ(storage.contains(3), true);
                ASSERT_EQ(storage.contains(4), true);

                ASSERT_EQ(s2.size(), 20);
                ASSERT_EQ(s2.contains(1), true);
                ASSERT_EQ(s2.contains(3), false);
                ASSERT_EQ(s2.contains(4), false);
            }
        }
        SECTION("test maybeDelete") {
            ASSERT_EQ(storage.contains(3), true);
            ASSERT_EQ(storage.maybeDelete(3), true);
            ASSERT_EQ(storage.maybeDelete(3), false);
            ASSERT_EQ(storage.contains(3), false);
        }
        SECTION("test clear()") {
            SECTION("insert new elements") {
                ASSERT_EQ(storage.maybeInsert(5, keys[2]), true);
                ASSERT_EQ(storage.maybeInsert(6, keys[3]), true);
            }
            SECTION("before clear") {
                SECTION("check iterators") {
                    ASSERT_NE(storage.begin(), storage.end());
                    ASSERT_NE(storage.cbegin(), storage.cend());
                }
                SECTION("check elements set") {
                    ASSERT_EQ(storage.contains(4), true);
                    ASSERT_EQ(storage.contains(5), true);
                    ASSERT_EQ(storage.contains(6), true);
                }
                SECTION("check count") {
                    SECTION("using iterators") {
                        size_t count = 0;
                        for (const auto& kv : storage) {
                            ++count;
                        }
                        ASSERT_EQ(count, 3);
                    }
                    SECTION("using iteration") {
                        size_t count = 0;
                        for (auto i = storage.size(); i --> 0; ) {
                            if (storage.contains(i)) ++count;
                        }
                        ASSERT_EQ(count, 3);
                    }
                }
            }
            storage.clear();

            SECTION("after clear") {
                SECTION("check iterators") {
                    ASSERT_EQ(storage.begin(), storage.end());
                    ASSERT_EQ(storage.cbegin(), storage.cend());
                }
                SECTION("check elements not set") {
                    ASSERT_EQ(storage.contains(4), false);
                    ASSERT_EQ(storage.contains(5), false);
                    ASSERT_EQ(storage.contains(6), false);
                }
                SECTION("check count") {
                    SECTION("using iterators") {
                        size_t count = 0;
                        for (const auto& kv : storage) {
                            ++count;
                        }
                        ASSERT_EQ(count, 0);
                    }
                    SECTION("using iteration") {
                        size_t count = 0;
                        for (auto i = storage.size(); i --> 0; ) {
                            if (storage.contains(i)) ++count;
                        }
                        ASSERT_EQ(count, 0);
                    }
                }
                SECTION("check cannot delete deleted elements") {
                    ASSERT_EQ(storage.maybeDelete(4), false);
                    ASSERT_EQ(storage.maybeDelete(5), false);
                    ASSERT_EQ(storage.maybeDelete(6), false);
                }
                SECTION("check can reinsert elements") {
                    ASSERT_EQ(storage.maybeInsert(4, keys[0]), true);
                    ASSERT_EQ(storage.maybeInsert(5, keys[1]), true);
                    ASSERT_EQ(storage.maybeInsert(6, keys[2]), true);
                }
            }
        }
    }
    SECTION("Testing " << name) {
        auto dict = hashtable.create(1, 0.8);
        SECTION("Should initially be empty") {
            ASSERT_EQ(dict.size(), 0);
            ASSERT_EQ(bool(dict), false);
            ASSERT_EQ(dict.begin(), dict.end());
        }
        SECTION("Test insertion") {
            dict[keys[0]] = values[0];
            ASSERT_EQ(dict[keys[0]], values[0]);
            ASSERT_EQ(dict.size(), 1);
            ASSERT_EQ(bool(dict), true);

            auto it = dict.begin();
            ASSERT_NE(it, dict.end());
            ASSERT_EQ(it->first, keys[0]);
            ASSERT_EQ(it->second, values[0]);
            ++it;
            ASSERT_EQ(it, dict.end());

            dict[keys[1]] = values[1];
            ASSERT_EQ(dict[keys[1]], values[1]);
            ASSERT_EQ(dict.size(), 2);

            it = dict.begin();
            ASSERT_NE(it, dict.end());
            ASSERT_EQ(it->first, keys[0]);
            ASSERT_EQ(it->second, values[0]);
            ++it;
            ASSERT_NE(it, dict.end());
            ASSERT_EQ(it->first, keys[1]);
            ASSERT_EQ(it->second, values[1]);
            ++it;
            ASSERT_EQ(it, dict.end());
            if (it != dict.end()) {
                info() << it->first << ", " << it->second;
            }
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
        SECTION("Test key find") {
            ASSERT_NE(dict.find(keys[0]), dict.end());
            ASSERT_EQ(dict.find(keys[0])->second, values[1]);
            ASSERT_NE(dict.find(keys[1]), dict.end());
            ASSERT_EQ(dict.find(keys[1])->second, values[1]);
            ASSERT_EQ(dict.find(keys[2]), dict.end());

            dict.find(keys[0])->second = values[3];
            ASSERT_EQ(dict[keys[0]], values[3]);
        }
        SECTION("Test key removal") {
            dict.deleteKey(keys[0]);
            ASSERT_EQ(dict.size(), 1);
            ASSERT_EQ(dict.containsKey(keys[0]), false);

            auto it = dict.begin();
            ASSERT_NE(it, dict.end());
            ASSERT_EQ(it->first, keys[1]);
            ++it;
            ASSERT_EQ(it, dict.end());
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
        SECTION("Test resize") {
            dict.resize(10);
            ASSERT_EQ(dict.size(), 4);
            ASSERT_EQ(dict[keys[0]], values[0]);
            ASSERT_EQ(dict[keys[1]], values[2]);
            ASSERT_EQ(dict[keys[2]], values[2]);
            ASSERT_EQ(dict[keys[3]], values[3]);
        }
        SECTION("Test copy-construction") {
            decltype(dict) second { dict };
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
            auto second = dict.create(1);
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
            const auto& constref = dict;
            ASSERT_EQ(constref[keys[0]], Value());
            ASSERT_EQ(constref.size(), 0);
        }
        SECTION("non-const operator[] should insert") {
            ASSERT_EQ(dict[keys[0]], Value());
            ASSERT_EQ(dict.size(), 1);
        }
    }
}

#define TEST_HT_IMPL(K, V, keys, values) \
    _testHTImpl("HashTable<" #K ", " #V ">", \
        make_hashtable<K,V>(std::hash<K>{}, 1), keys, values)

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

    #define TEST_WITH_KEYS(K, keys) \
        TEST_HT_IMPL(K, int, keys, ints); \
        TEST_HT_IMPL(K, double, keys, doubles); \
        TEST_HT_IMPL(K, char, keys, chars); \
        TEST_HT_IMPL(K, std::string, keys, strings);
        // TEST_HT_IMPL(K, pair, keys, pairs);

    #define TEST_HT() \
        TEST_WITH_KEYS(int, ints) \
        TEST_WITH_KEYS(double, doubles) \
        TEST_WITH_KEYS(char, chars) \
        TEST_WITH_KEYS(std::string, strings)
        // TEST_WITH_KEYS(pair, pairs)

    SECTION("All tests") {
        TEST_HT()
    }
    std::cout << "\n\033[32mAll tests passed\n\033[0m";
    return 0;
}
