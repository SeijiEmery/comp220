// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// StaticArray.hpp
// Implements a templated static array with non-throwing bounds checking.
//
// Remote source:
// https://github.com/SeijiEmery/comp220/tree/master/assignment_02/src/StaticArray.hpp
//

#ifndef StaticArray_h
#define StaticArray_h

template <typename T, size_t N>
class StaticArray {
    T _data[N];
    T _dummy;
public:
    // Constructors, assignment operators
    StaticArray () : _data(), _dummy() {}
    StaticArray (const StaticArray&) = default;
    StaticArray& operator= (const StaticArray&) = default;

    // Get array capacity
    int capacity () const { return static_cast<int>(N); }

    // Get / set array elements. Elements out of range returns reference to dummy variable.
    const T& operator[] (int i) const { return i < 0 || i > N ? _dummy : _data[i]; }
    T& operator[] (int i)             { return i < 0 || i > N ? (_dummy = {}) : _data[i]; }
};

#endif //StaticArray_h
