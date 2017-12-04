// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// SortableArray.h
// Implements a templated sortable dynamic array with non-throwing bounds checking.
//
// Remote source:
// https://github.com/SeijiEmery/comp220/tree/master/assignment_12/src/SortableArray.hpp
//

#ifndef SortableArray_h
#define SortableArray_h
#include <cassert>

template <typename T>
class SortableArray {
    size_t _capacity = 0;
    T*     _data = nullptr;           // dummy incorporated into array at index N. (reserves N+1 elements for data)
    T      _dummy;
    
public:
    // Constructors, assignment operators, destructor
    SortableArray (size_t capacity = 2);
    SortableArray (const SortableArray<T>&);
    SortableArray& operator= (const SortableArray<T>&);
    ~SortableArray ();

    // Move operations
    SortableArray (SortableArray<T>&& other) { *this = std::move(other); }
    SortableArray<T>& operator= (SortableArray<T>&& other) { 
        std::swap(_capacity, other._capacity);
        std::swap(_data, other._data);
        std::swap(_dummy, other._dummy);
        return *this; 
    }

    // Get / set array capacity
    void capacity (size_t cap);
    size_t capacity () const { return _capacity; }

    // Get / set array elements. Elements out of range returns reference to dummy variable.
    const T& operator[] (int i) const;
    T& operator[] (int i);
};

//
// SortableArray implementation
//

// Helper functions, equivalent to std::copy / std::fill
namespace detail {
    template <typename ForwardIt, typename T>
    void fill (ForwardIt first, ForwardIt last, const T& value) {
        for (; first != last; ++first) {
            *first = value;
        }
    }
    template <typename InputIterator, typename OutputIterator>
    OutputIterator copy (InputIterator first, InputIterator last, OutputIterator out) {
        for (; first != last; ++first, ++out) {
            *out = *first;
        }
        return out;
    }
    // Round number up to next power of 2
    size_t nextPow2 (size_t n) {
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return ++n;
    }
}; // namespace detail

template <typename T>
SortableArray<T>::SortableArray (size_t count) 
    : _capacity(count),
      _data(new T[_capacity]),
      _dummy(T())
{
    // std::cout << "\033[36mALLOC " << _capacity << " (" << _capacity * sizeof(T) << ")\033[0m\n";
    detail::fill(&_data[0], &_data[_capacity], T());
}

template <typename T>
SortableArray<T>::SortableArray (const SortableArray<T>& other) 
    : _capacity(other._capacity),
      _data(new T[_capacity]),
      _dummy(T())
{
    // std::cout << "\033[36mALLOC " << _capacity << " (" << _capacity * sizeof(T) << ")\033[0m\n";
    detail::copy(&other._data[0], &other._data[_capacity], &_data[0]);
}

template <typename T>
SortableArray<T>& SortableArray<T>::operator= (const SortableArray<T>& other) {
    if (this != &other) {
        // std::cout << "\033[35mDEALLOC " << _capacity << " (" << _capacity * sizeof(T) << ")\033[0m\n";
        delete[] _data;

        _capacity = other._capacity;
        _data = new T[_capacity];
        _dummy = T();
        // std::cout << "\033[36mALLOC " << _capacity << " (" << _capacity * sizeof(T) << ")\033[0m\n";
        detail::copy(&other._data[0], &other._data[_capacity], &_data[0]);
    }
    return *this;
}

template <typename T>
SortableArray<T>::~SortableArray () {
    if (_data) {
        delete[] _data;
        _data = nullptr;
        // std::cout << "\033[35mDEALLOC " << _capacity << " (" << _capacity * sizeof(T) << ")\033[0m\n";
    }
}

template <typename T>
void SortableArray<T>::capacity (size_t cap) {
    if (cap < 10) cap = 10;

    // std::cout << "\033[34mresizing " << _capacity << " => " << cap << "\033[0m\n";

    if (cap > _capacity) {
        // Allocate new array
        T* newData = new T[cap];
        // std::cout << "\033[36mALLOC " << _capacity << " (" << _capacity * sizeof(T) << ")\033[0m\n";

        // Copy existing data
        if (_capacity != 0) {
            detail::copy(&_data[0], &_data[_capacity], &newData[0]);
        }
        // Fill remaining data with zeroes
        assert(_capacity < cap);
        detail::fill(&newData[_capacity], &newData[cap], T());

        // Delete old array, and replace array / capacity
        if (_data != nullptr) {
            delete[] _data;
            _data = nullptr;
            // std::cout << "\033[35mDEALLOC " << _capacity << " (" << _capacity * sizeof(T) << ")\033[0m\n";
        }
        _data = newData;
        _capacity = cap;
    }
}

template <typename T>
const T& SortableArray<T>::operator[] (int i) const {
    return i < 0 || i > _capacity ? _dummy : _data[i];
}
template <typename T>
T& SortableArray<T>::operator[] (int i) {
    if (i < 0) return _dummy = {};
    if (i >= _capacity) {
        // Resize capacity
        capacity(detail::nextPow2(i+1));
        assert(_capacity >= i);
    }
    return _data[i]; 
}

#endif //SortableArray_h
