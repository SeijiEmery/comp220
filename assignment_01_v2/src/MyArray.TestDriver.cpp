// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// MyArray.TestDriver.cpp
// Tests our array implementation.
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
// https://github.com/SeijiEmery/comp220/tree/master/assignment_01_v2/src/MyArray.TestDriver.cpp
//

#include <iostream>     // cerr, cout
#include <string>       // string
using namespace std;

//
// Array implementation
//

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
};

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
    return !(index < 0 || index > capacity()) ? _data[index] : _dummy;
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
// Test driver program
//

int main () {
    std::cout << "Programmer: Seiji Emery\n";
    std::cout << "Programmer ID: M00202623\n";
    std::cout << "File: " __FILE__ "\n";

    Array a;

    SECTION("Array should be default-constructed to 100 elements") {
        ASSERT_EQ(a.capacity(), 100);
    }
    SECTION("Array elements should be default-initialized to 0") {
        int numNonEqualElements = 0;
        for (auto i = 0; i < a.capacity(); ++i) {
            if (a[i] != 0) ++numNonEqualElements;
        }
        ASSERT_EQ(numNonEqualElements, 0);
    }
    SECTION("Test Array::operator[] getter / setter") {
        a[6] = 12356;
        a[7] = 7654321;
        ASSERT_EQ(a[6], 12356);
        ASSERT_EQ(a[7], 7654321);
    
        int* dummy_ptr = &(a[-1000]);           // get address of dummy pointer
    
        a[-1000] = 123123;
        ASSERT_EQ(*dummy_ptr, 123123);         // check value is written
        ASSERT_EQ(&(a[-6]), &(a[-1000]));      // any out-of-range uses dummy
    
        ASSERT_EQ(a[-6], 123123);          // any out-of-range uses dummy
        ASSERT_EQ(a[100], 123123);         // checks upper end of range
        ASSERT_NE(a[9],  123123);          // checks upper end of range
        ASSERT_NE(a[0],  123123);          // checks lower end of range
    }
    SECTION("Const-object test") {
        const Array b = a;

        ASSERT_EQ(b.capacity(), 100);
        ASSERT_EQ(b[0], b[0]);
        ASSERT_EQ(b[-1], b[-1]);

        int numNonEqualElements = 0;
        for (auto i = 0; i < a.capacity(); ++i) {
            if (a[i] != b[i]) ++numNonEqualElements;
        }
        ASSERT_EQ(numNonEqualElements, 0);
    }
    reportTestResults();
    std::cout << "\033[32mAll tests passed\n\033[0m";
    return 0;
}
