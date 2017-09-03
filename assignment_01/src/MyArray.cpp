// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Week 1, a very simple static array implementation.
// 
// Includes proper unittesting (catch.hpp, from https://github.com/philsquared/Catch)
// compile with g++ -Wall -std=c++1z ../src/MyArray.cpp; ./a.out
//
// Compile flags (optional, ignore these):
//  -D RUN_TESTS            (run tests at program start, disabled by default)
//  -D USE_CATCH            (use catch.hpp instead of fallback tests, disabled by default)
//  -D USE_MIN_TEST_FRAMEWORK   (use a custom "framework" for our fallback tests, else use assert; disabled by default)
//  -D NO_MAIN              (don't run main program, disabled by default)
//  -D ALWAYS_RETURN_ZERO   (out-of-bounds array accesses always return 0, disabled by default). [1]
// 
// [1] Contradicts assignment spec, but I strongly prefer this behavior, and it's trivial 
// to implement (return _dummy = 0). Defaults to assignment spec, but can be switched with 
// this #define. All unittests have been written to support both versions, and it makes no 
// difference to the actual program.
//

// Edit: contrary to the above, I'm turning this on by default.
// If it is not on, values WILL be bugged for the keys array and return false-positives.
// You can turn this off by commenting out the following line; array will perform according to spec,
// but program behavior will be somewhat broken.
#define ALWAYS_RETURN_ZERO

#ifdef _WIN32
    #ifdef USE_CATCH
        #undef USE_CATCH
        #define USE_MIN_TEST_FRAMEWORK
    #endif
#endif
#if defined(SEMERY_IMPL) || !defined(USE_CATCH)

#include <iostream>     // std::cin, std::cout, std::endl
#include <cstdlib>      // atoi(), memcpy(), memset()
#include <string>       // std::string

#ifdef USE_CATCH
    #define CATCH_CONFIG_RUNNER
    #include "catch.hpp"
#endif

using namespace std;

// Array class, as specified by this assignment
class Array {
    enum { SIZE = 100 };
    int _data[SIZE];    // data storage (fixed)
    int _dummy = 0;     // used to implement safe out-of-bounds array access (when accessing an element by reference)
                        // for array element lookup
public:
    // Default constructor, initializes all values to 0.
    Array ();

    // Copy constructor + assignment operator
    Array (const Array& array);
    Array& operator= (const Array& array);

    // Get array's capacity (hardcoded)
    size_t capacity () const { return SIZE; }

    // get / set array elements
    int operator[](int index) const;
    int& operator[](int index);

    // Iterator functionality for c++11 range-based for (just uses raw pointers)
    int* begin () { return &_data[0]; }
    int* end   () { return &_data[capacity()]; }

    const int* begin () const { return &_data[0]; }
    const int* end   () const { return &_data[capacity()]; }
    
    const int* cbegin () const { return begin(); }
    const int* cend   () const { return end(); }
};

//
// Array implementation
//

// Helper methods, equivalent of std::copy, std::fill, specialized for fixed arrays.
// I'm assuming that memset / memcpy may be used in this course.
template <typename T, size_t N>
void copy (T (&dst)[N], const T (&src)[N]) {    // Really funky syntax for specifying references to fixed-size arrays 
    memcpy(static_cast<void*>(&dst[0]), static_cast<const void*>(&src[0]), N * sizeof(T));
}
template <typename T, size_t N>
void fill (T (&dst)[N], T value) {
    memset(static_cast<void*>(&dst[0]), value, N * sizeof(T));
}

// Constructors
Array::Array () { fill(_data, 0); }
Array::Array (const Array& array) { copy(_data, array._data); }
Array& Array::operator= (const Array& array) { return copy(_data, array._data), *this; }

// Get / set array elements
int Array::operator[] (int index) const { 
    return !(index < 0 || index > capacity()) ? _data[index] : 0; 
}
int& Array::operator[] (int index) {
    #ifdef ALWAYS_RETURN_ZERO
        return !(index < 0 || index > capacity()) ? _data[index] : _dummy = 0;
    #else
        return !(index < 0 || index > capacity()) ? _data[index] : _dummy;
    #endif
}

// Print elements (for debugging purposes)
auto& operator << (std::ostream& os, const Array& array) {
    os << "[ ";
    for (auto& value : array) {
        os << value << ", ";
    }
    return os << "\b\b]";   // note: backspace character is a thing, fine if displaying to terminal
}

//
// Array unittests
//

#ifdef RUN_TESTS
#ifdef USE_CATCH // Better unittests using catch.hpp
void runTests (int argc, const char** argv) {
    if (int error = Catch::Session().run(argc, argv)) {
        exit(error);
    }
}
TEST_CASE("Array meets specifications", "[Array]") {
    Array array;
    SECTION("Array should be default-constructed to 100 elements") {
        REQUIRE(array.capacity() == 100);
    }
    SECTION("Array elements should be default-initialized to 0") {
        for (auto i = 0; i < array.capacity(); ++i) {
            REQUIRE(array[i] == 0);
        }
    }
    // Set array value, use this in following tests to check assignment / copy construction.
    array[9] = 100;
    REQUIRE(array[9] == 100);

    SECTION("Should support range-based-for iteration") {
        int i = 0;
        for (auto& value : array) {
            REQUIRE((value == 100) == (i++ == 9));
        }
    }
    SECTION("Should support range-based-for const iteration") {
        int i = 0;
        for (const auto& value : array) {
            REQUIRE((value == 100) == (i++ == 9));
        }
    }
    SECTION("Array const-copy-construct works correctly") {
        const Array array2 = array;
        REQUIRE(array2[0] == 0);
        REQUIRE(array2[9] == 100);
    }
    SECTION("Array copy does not modify same memory") {
        Array array3 = array;
        REQUIRE(array3[0] == 0);
        REQUIRE(array3[9] == 100);
        array3[9] = -27;
        REQUIRE(array3[9] == -27);
        REQUIRE(array[9] == 100);
    }
    SECTION("Array checks bounds on read and returns 0 when failing bounds check") {
        for (auto i = -10; i < 120; ++i) {
            array[i] = 42;
        }
        REQUIRE(array[0] == 42);
        REQUIRE(array[array.capacity()-1] == 42);
        // REQUIRE(array[-1] == 0);
        // REQUIRE(array[array.capacity()] == 0);
        REQUIRE(array[-1] == 42);
        REQUIRE(array[array.capacity()] == 42);
    }
    SECTION("Array writes to dummy variable on out-of-bounds-write") {
        array[227] = -29;           // write to dummy variable
        #ifdef ALWAYS_RETURN_ZERO
            // should always return zero, even when reading from dummy variable
            REQUIRE(array[227] == 0);   
        #else
            // should return last value read from dummy variable (I don't like this behavior, but it's in the spec...)
            REQUIRE(array[227] == -29); 
        #endif

        // This should always work, hoewever
        int* value = &array[-19];   // perform tricks to get dummy variable
        array[227] = -29;           // should write to dummy variable
        REQUIRE(*value == -29);     // and check that we are writing to the same one for all out of bounds
                                    // writes (and not some random location in memory or something).
    }
}

#elif defined(USE_MIN_TEST_FRAMEWORK)  // fallback unittests (write our own framework)

#include <sstream>

template <typename A, typename B>
void raiseAssertionError (const char* msg, A first, B second, size_t line, const char* file) {
    std::stringstream ss;
    ss << msg << "\n\texpected: " << first << "\n\tnot:      " << second << "\n\ton file " << file << ":" << line;
    throw std::logic_error(ss.str());
}
#define _ASSERT_THAT(expr, a, b) do { \
    if (!(expr)) { \
        raiseAssertionError("Assertion failure " #expr ": ", a, b, __LINE__, __FILE__); \
    } \
} while(0)
#define REQUIRE_EQ(a, b) _ASSERT_THAT(a == b, a, b)
#define REQUIRE_NE(a, b) _ASSERT_THAT(a != b, a, b)
#define REQUIRE_GT(a, b) _ASSERT_THAT(a > b, a, b)
#define REQUIRE_LT(a, b) _ASSERT_THAT(a < b, a, b)
#define REQUIRE_GE(a, b) _ASSERT_THAT(a >= b, a, b)
#define REQUIRE_LE(a, b) _ASSERT_THAT(a <= b, a, b)

template <typename Ex>
bool guardException (const std::function<void()>& onError, const std::function<bool()>& closure) {
    try {
        return closure();
    } catch (Ex& exception) {
        std::cerr << exception.what() << std::endl;
        onError();        
        return false;
    }
    return true;
}
#define TEST_SECTION(name) guardException<std::logic_error>([](){ \
    std::cerr << "Test failed: " name << " (file " << __FILE__ << ":" << __LINE__ << ')' << std::endl; }, \
    [&]() -> bool {

#define END_SECTION \
    return true; \
})

void runTests (int argc, const char** argv) {
    // Behold, the fugliness that is a hacked-together, minimally functional unit-test framework (that doesn't pollute std::cout...)
    if (!TEST_SECTION("Array tests") {
        Array a;
        return TEST_SECTION("Array::capacity") {
            REQUIRE_EQ(a.capacity(), 100);            
        } END_SECTION && TEST_SECTION("Array::Array") {
            for (int i = 0; i < a.capacity( ); i++) {
                REQUIRE_EQ(a[i], 0);
            }
        } END_SECTION && TEST_SECTION("Array::operator[] setter") {
            a[6] = 12356;
            a[7] = 7654321;
            REQUIRE_EQ(a[6], 12356);
            REQUIRE_EQ(a[7], 7654321);
        
            int* dummy_ptr = &(a[-1000]);           // get address of dummy pointer
        
            a[-1000] = 123123;
            REQUIRE_EQ(*dummy_ptr, 123123);         // check value is written
            REQUIRE_EQ(&(a[-6]), &(a[-1000]));      // any out-of-range uses dummy
        
            #ifndef ALWAYS_RETURN_ZERO
                REQUIRE_EQ(a[-6], 123123);          // any out-of-range uses dummy
                REQUIRE_EQ(a[100], 123123);         // checks upper end of range
                REQUIRE_NE(a[9],  123123);          // checks upper end of range
                REQUIRE_NE(a[0],  123123);          // checks lower end of range
            #endif
        } END_SECTION && TEST_SECTION("Array::operator[] getter") {
            const Array b = a;
            for (int i = 0; i < 10; i++) {
                REQUIRE_EQ(a[i], b[i]);
            }
        } END_SECTION && TEST_SECTION("Const object test") {
            const Array c;                  // if this compiles, Array::Array main constructor exists 
            REQUIRE_EQ(c.capacity(), 100);  // if this compiles, Array::capacity is a getter 
            REQUIRE_EQ(c[0], c[0]);         // if this compiles, there is an Array::operator[ ] getter 
            REQUIRE_EQ(c[-1], c[-1]);       // tests the getter's range checking 
        } END_SECTION;
    } END_SECTION) {
        std::cout << "Test(s) failed\n";
        exit(-1);
    } else {
        std::cout << "All tests passed\n";
    }
}

#else // Fallback unittests (no framework, just use assert() + std::cout-polluting debugging statements)

#include <cassert>    
void runTests (int argc, const char** argv) {        
    #define REQUIRE_TEST(expr, expected, test) do {\
        cout << "EXPECTED: " << expected << " for " #test "\n"; \
        auto value = expr; \
        cout << "ACTUAL:   " << value << '\n'; \
        assert(test); \
    } while(0)
    #define REQUIRE_EQ(expr, expected) REQUIRE_TEST(expr, expected, expr == expected)
    #define REQUIRE_NE(expr, expected) REQUIRE_TEST(expr, expected, expr != expected)

    std::cout << "Running tests\n";
    Array a;

    cout << "\nTesting Array::capacity\n";
    REQUIRE_EQ(a.capacity(), 100);

    cout << "\nTesting Array::Array\n";
    for (int i = 0; i < a.capacity( ); i++)
        assert(a[i] == 0);

    std::cout << "\nTesting the Array::operator[ ] setter\n";
    a[6] = 12356;
    a[7] = 7654321;
    REQUIRE_EQ(a[6], 12356);
    REQUIRE_EQ(a[7], 7654321);

    int* dummy_ptr = &(a[-1000]);       // get address of dummy pointer

    a[-1000] = 123123;
    REQUIRE_EQ(*dummy_ptr, 123123);     // check value is written
    REQUIRE_EQ(&(a[-6]), &(a[-1000]));  // any out-of-range uses dummy

    #ifndef ALWAYS_RETURN_ZERO
        REQUIRE_EQ(a[-6], 123123);          // any out-of-range uses dummy
        REQUIRE_EQ(a[100], 123123);         // checks upper end of range
        REQUIRE_NE(a[9],  123123);          // checks upper end of range
        REQUIRE_NE(a[0],  123123);          // checks lower end of range
    #endif

    // Array::operator[ ] getter
    cout << "\nTesting the Array::operator[ ] getter\n";
    const Array b = a;
    for (int i = 0; i < 10; i++) {
        assert(a[i] == b[i]);
    }

    // const object test
    cout << "\nConst object test\n";
    const Array c; // if this compiles, Array::Array main constructor exists 
    assert(c.capacity( )); // if this compiles, Array::capacity is a getter 
    assert(c[0] == c[0]); // if this compiles, there is an Array::operator[ ] getter 
    assert(c[-1] == c[-1]); // tests the getter's range checking 

    // #undef REQUIRE_EQ

    // std::cout << "Finished tests\n\n";
}

#endif // USE_CATCH
#endif // RUN_TESTS

//
// Main program / application
//

int main (int argc, const char** argv) {
    std::cout << "Programmer:       Seiji Emery\n";
    std::cout << "Programmer's ID:  M00202623\n";
    std::cout << "File:             " << __FILE__ << '\n' << std::endl;

    #ifdef RUN_TESTS
        runTests(argc, argv);
    #endif
    #ifndef NO_MAIN

    // Run main program
    Array keys;   
    Array values; 
    std::string input;

    do {
        std::cout << "Input an index and a value [Q to quit]: ";
        std::cin >> input;
        if (input[0] == 'Q' || input[0] == 'q') {
            break;
        }
        int index = atoi(input.c_str());
        std::cin >> input;
        int value = atoi(input.c_str());

        keys[index]   = 1;
        values[index] = value;
    } while (1);

    // Calculate unique user inputs
    int uniqueValuesEntered = 0;
    for (auto i = 0; i < keys.capacity(); ++i) {
        if (keys[i] != 0) {
            ++uniqueValuesEntered;
        }
    }

    // Display all inputs
    std::cout << "\nYou stored this many values: " << uniqueValuesEntered << '\n';
    std::cout << "The index-value pairs are:\n";
    for (auto i = 0; i < keys.capacity(); ++i) {
        if (keys[i] != 0) {
            std::cout << ' ' << i << " => " << values[i] << '\n';
        }
    }
    std::cout << '\n';

    // Interactively querry for inputs
    do {
        std::cout << "Input an index for me to look up [Q to quit]: ";
        std::cin >> input;
        if (input[0] == 'Q' || input[0] == 'q') {
            break;
        }
        int index = atoi(input.c_str());
        if (keys[index]) {
            std::cout << "Found it -- the value stored in " << index << " is " << values[index] << '\n';
        } else {
            std::cout << "I didn't find it\n";
        }
    } while (1);
    #endif // NO_MAIN
    return 0;
}

#else

#include <cstdlib>
#include <cstring>

int main () {
    // Fetch catch.hpp (and latest source) and rebuild.
    // Assumes unix, so disabled by #ifdefs above on windows.
    system("mkdir -p .temp");
    #ifdef USE_LOCAL_SRC
        system("cp ../src/MyArray.cpp .temp/MyArray.cpp");
    #else
        system("wget -nc -q -O .temp/MyArray.cpp https://raw.githubusercontent.com/SeijiEmery/comp220/master/assignment_01/src/MyArray.cpp");
    #endif
    char build_cmd[1024] = "c++ -Wall -std=c++1z .temp/MyArray.cpp -o .temp/myarray -D SEMERY_IMPL\0";
    #ifdef RUN_TESTS
        strncat(&build_cmd[0], " -D RUN_TESTS", 1024);
        #ifdef USE_CATCH
            system("wget -nc -q -O .temp/catch.hpp https://raw.githubusercontent.com/philsquared/Catch/master/single_include/catch.hpp");
            strncat(&build_cmd[0], " -D USE_CATCH", 1024);
        #endif
    #endif
    #ifdef NO_MAIN
        strncat(&build_cmd[0], " -D NO_MAIN", 1024);
    #endif

    system(&build_cmd[0]);
    return system(".temp/myarray");
}
#endif
