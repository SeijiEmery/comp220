// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Week 1, a very simple static array implementation.
// 
// Remote source:
// https://github.com/SeijiEmery/comp220/tree/master/assignment_01_v2/src/MyArray.cpp
//

#include <iostream>     // std::cin, std::cout, std::endl
#include <string>       // std::string
using namespace std;

#include <cstdlib>      // atoi(), memcpy(), memset()

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
// Main program / application
//

int main (int argc, const char** argv) {
    std::cout << "Programmer:       Seiji Emery\n";
    std::cout << "Programmer's ID:  M00202623\n";
    std::cout << "File:             " << __FILE__ << '\n' << std::endl;

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

    keys[-1] = 0;   // clear out of bounds keys...

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
    return 0;
}
