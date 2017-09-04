// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// StaticArray.TestDriver.cpp
// Tests our static array implementation.
//

#include <iostream>     // cerr, cout
#include <string>       // string
#include <cmath>        // fabs
#include <cstring>
using namespace std;

#include "StaticArray.hpp"
#include "StaticArray.hpp" // multiple include test

template <typename T, size_t N>
void _testArrayImpl (const char*, T, T, T, T);

#define TEST_ARRAY_IMPL(T, first, second, third) \
    _testArrayImpl<T,100>(#T, {}, first, second, third)

int main () {
    TEST_ARRAY_IMPL(int, 1, 2, 3);
    TEST_ARRAY_IMPL(double, 1.5, 2.5, 3.5);
    TEST_ARRAY_IMPL(char, 'a', '@', 'Z');
    TEST_ARRAY_IMPL(std::string, "foo", "bar", "baz");
    std::cout << "All tests passed\n";
    return 0;
}


int cmp (int a, int b)   { return a - b; }
int cmp (char a, char b) { return a - b; }
int cmp (double a, double b) {
    return (fabs(a - b) < 1e-20) ? 0 : a > b ? 1 : -1;
}
int cmp (const std::string& a, const std::string& b) {
    return strcmp(a.c_str(), b.c_str());
}


//
// Mini-testing framework (©2017 Seiji Emery)
//
#include <vector>       // vector
#include <functional>   // function
#include <cstdlib>      // exit

// Setup unittest dependencies
namespace detail {
    std::vector<std::function<void()>> g_testScope;

    struct TestScopeInserter {
        TestScopeInserter (std::function<void()> scope) { /*std::cerr << "begin scope: "; scope();*/ g_testScope.push_back(scope); }
        ~TestScopeInserter () { /*std::cerr << "end scope: "; g_testScope.back();*/ g_testScope.pop_back(); }
        operator bool () const { return true; }
    };
    template <typename A, typename B>
    void signalAssertFailure (const char* msg, const A& a, const B& b) {
        std::cerr << msg << " (" << a << ", " << b << ", cmp: " << cmp(a,b) << "), file " << __FILE__ << ':' << __LINE__ << '\n';
        for (auto i = g_testScope.size(); i --> 0; ) {
            g_testScope[i]();
        }
        std::cerr.flush();
        exit(-1);
    }
}; // namespace detail

#define SECTION(args...) if (auto _  = detail::TestScopeInserter([=](){ \
    std::cerr << "in section " << args << '\n'; \
}))

#define REQUIRE_THAT(expr, cond, a, b) do { \
    if (!(cond)) { \
        detail::signalAssertFailure("Test failed: " #expr, a, b); \
    } \
} while (0)

#define REQUIRE_EQ(a,b) REQUIRE_THAT(a == b, cmp(a,b) == 0, a, b)
#define REQUIRE_NE(a,b) REQUIRE_THAT(a != b, cmp(a,b) != 0, a, b)
#define REQUIRE_GE(a,b) REQUIRE_THAT(a >= b, cmp(a,b) >= 0, a, b)
#define REQUIRE_LE(a,b) REQUIRE_THAT(a <= b, cmp(a,b) <= 0, a, b)
#define REQUIRE_GT(a,b) REQUIRE_THAT(a > b, cmp(a,b) > 0, a, b)
#define REQUIRE_LT(a,b) REQUIRE_THAT(a < b, cmp(a,b) < 0, a, b)

//
// End mini-testing framework
//


//
// Test implementation
//

template <typename T, size_t N>
void _testArrayImpl (const char* name, T init, T first, T second, T third) {
    SECTION("Testing StaticArray<" << name << ", " << N << ">") {
        SECTION("Sanity check") {
            REQUIRE_EQ(first, first);
            REQUIRE_GE(first, first);
            REQUIRE_LE(first, first);
            REQUIRE_NE(first, second);
            REQUIRE_LT(first, second);
            REQUIRE_GT(second, first);            
        }
        StaticArray<T,N> array;
        std::fill(&array[0], &array[N], init);

        SECTION("Testing StaticArray capacity (should equal " << N << ")") {
            REQUIRE_EQ(array.capacity(), N);
        }
        SECTION("Testing StaticArray initial values (should equal " << init << ")") {
            for (auto i = 0; i < array.capacity(); ++i) {
                std::cout << "i = " << i << ", array[i] = " << array[i] << '\n';
                REQUIRE_EQ(array[i], init);
            }
        }
        SECTION("Testing StaticArray getter / setter") {
            array[0] = first;
            REQUIRE_EQ(array[0], first);

            array[13] = second;
            REQUIRE_EQ(array[13], second);

            array[N-1] = third;
            REQUIRE_EQ(array[N-1], third);
        }
        SECTION("Testing out-of-bounds array values") {
            array[-1] = first;
            REQUIRE_EQ(array[-1], first);
            REQUIRE_EQ(array[N],  first);
            REQUIRE_NE(array[N], array[N-1]);
        }

        SECTION("Const-object test") {
            const StaticArray<T, N> array2 = array;
            REQUIRE_EQ(array2[0], first);
            REQUIRE_EQ(array[13], second);
            REQUIRE_EQ(array[N-1], third);
        }
    }
}
