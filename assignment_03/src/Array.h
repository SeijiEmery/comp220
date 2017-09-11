// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Array.h
// Implements a templated dynamic array with non-throwing bounds checking.
//
// Remote source:
// https://github.com/SeijiEmery/comp220/tree/master/assignment_03/src/Array.hpp
//

#ifndef Array_h
#define Array_h

template <typename T>
class Array {
    size_t _capacity = 100;    
    T*     _data;           // dummy incorporated into array at index N. (reserves N+1 elements for data)

public:
    // Constructors, assignment operators
    Array ();
    Array (const Array<T>&);
    Array& operator= (const Array&);

    // Get array capacity
    int capacity () const { return _capacity; }

    // Get / set array elements. Elements out of range returns reference to dummy variable.
    const T& operator[] (int i) const;
    T& operator[] (int i);
};

//
// Array implementation
//

// template <typename ForwardIt, typename T>
// void fill (ForwardIt first, ForwardIt last, const T& value) {
//     for (; first != last; ++first) {
//         *first = value;
//     }
// }
// template <typename InputIterator, typename OutputIterator>
// OutputIterator copy (InputIterator first, InputIterator last, OutputIterator out) {
//     for (; first != last; ++first, ++out) {
//         *out = *first;
//     }
//     return out;
// }

template <typename T>
Array<T>::Array () 
    : _data(new T[_capacity+1])
{
    fill(&_data[0], &_data[_capacity], T());
}

template <typename T>
Array<T>::Array (const Array<T>& other) 
    : _capacity(other._capacity),
      _data(new T[_capacity+1])
{
    copy(&other[0], &other[-1], &_data[0]);
}

template <typename T>
Array<T>& Array<T>::operator= (const Array<T>& other) {
    if (this != &copy) {
        delete[] _data;

        _capacity = other._capacity;
        _data = new T[_capacity+1];
        copy(&other[0], &other[-1], &_data[0]);
    }
    return *this;
}

template <typename T>
const T& Array<T>::operator[] (int i) const {
    return i < 0 || i > _capacity ? _data[_capacity] : _data[i];
}
template <typename T>
T& Array<T>::operator[] (int i) {
    return i < 0 || i > _capacity ? _data[_capacity] : _data[i];
}

#endif //Array_h
