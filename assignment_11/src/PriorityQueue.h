
#ifndef PriorityQueue_h
#define PriorityQueue_h

#include <vector>
#include <cassert>

#ifndef NO_PQUEUE_DEBUG
#include <functional> // std::function
#endif

template <typename T>
class PriorityQueue {
public:
    typedef PriorityQueue<T> This;
private:
    std::vector<T> elements;

    #ifndef NO_PQUEUE_DEBUG
    std::function<void(const This&)> debugCallback;
    void debugSwapOperation () { if (debugCallback) { debugCallback(*this); } }
    #else
    void debugSwapOperation () {}
    #endif

    static size_t parent (size_t i) { return (i + 1) / 2 - 1; }
    static size_t left   (size_t i) { return i * 2 + 1; }
    static size_t right  (size_t i) { return i * 2 + 2; }

    void heapify (size_t i) {
        assert(i < size());
        while (i > 0 && elements[i] > elements[parent(i)]) {
            std::swap(elements[i], elements[parent(i)]); i = parent(i);
            debugSwapOperation();
        }
    }
public:
    PriorityQueue () {}
    PriorityQueue (const This& other) : elements(other.elements) {}
    This& operator= (const This& other) { return elements = other.elements, *this; }
    PriorityQueue (This&& other) : elements(std::move(other.elements)) {}
    This& operator= (This&& other) { return elements = std::move(other.elements), *this; }
    ~PriorityQueue () {}

    #ifndef NO_PQUEUE_DEBUG
    template <typename F>
    void debugOnSwap (F callback) { debugCallback = callback; debugSwapOperation(); }
    #endif

    friend bool operator == (const This& a, const This& b) { return a.elements == b.elements; }
    friend bool operator != (const This& a, const This& b) { return a.elements != b.elements; }
    friend bool operator >= (const This& a, const This& b) { return a.elements >= b.elements; }
    friend bool operator <= (const This& a, const This& b) { return a.elements <= b.elements; }
    friend bool operator >  (const This& a, const This& b) { return a.elements > b.elements; }
    friend bool operator <  (const This& a, const This& b) { return a.elements < b.elements; }

    size_t size () const { return elements.size(); }
    bool empty () const { return elements.empty(); }
    operator bool () const { return !elements.empty(); }

    void push (const T& value) {
        elements.push_back(value);
        heapify(size() - 1);
    }
    void push (T&& value) {
        elements.push_back(std::move(value));
        heapify(size() - 1);
    }
    template <typename... Args>
    void emplace (Args... args) {
        elements.emplace_back(args...);
        heapify(size() - 1);
    }
    T& peek () {
        assert(!empty());
        return elements[0];
    }
    const T& peek () const {
        assert(!empty());
        return elements[0];
    }
    void pop () {
        assert(!empty());
        size_t i = 0, n = size();
        while (i < n) {
            if (left(i) >= n && right(i) >= n) {
                break;
            } else if (right(i) >= n || (left(i) < n && elements[left(i)] >= elements[right(i)])) {
                debugSwapOperation();
                std::swap(elements[i], elements[left(i)]); i = left(i);
            } else {
                debugSwapOperation();
                std::swap(elements[i], elements[right(i)]); i = right(i);
            }
        }
        if (i < n - 1) {
            debugSwapOperation();
            std::swap(elements[i], elements[n - 1]);
            heapify(i);
        }
        debugSwapOperation();
        elements.pop_back();
    }
    void clear () {
        elements.clear();
    }

    friend std::ostream& operator<< (std::ostream& os, const This& queue) {
        if (queue) {
            os << "[ " << queue.elements[0];
            for (size_t i = 1; i < queue.size(); ++i) {
                os << ", " << queue.elements[i];
            }
            return os << " ]";
        } else {
            return os << "[]";
        }
    }

    typedef typename decltype(elements)::iterator iterator;
    typedef typename decltype(elements)::const_iterator const_iterator;

    iterator begin () { return elements.begin(); }
    iterator end   () { return elements.end();   }

    const_iterator begin () const { return elements.begin(); }
    const_iterator end   () const { return elements.end();   }
};

#endif // PriorityQueue_h
