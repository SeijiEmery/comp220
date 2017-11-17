
#ifndef PriorityQueue_h
#define PriorityQueue_h

#include <vector>
#include <cassert>

#define SET_COLOR(code) "\033[" code "m"
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


template <typename T>
class PriorityQueue {
    std::vector<T> elements;

    static size_t parent (size_t i) { return (i + 1) / 2 - 1; }
    static size_t left   (size_t i) { return i * 2 + 1; }
    static size_t right  (size_t i) { return i * 2 + 2; }

    void heapify (size_t i) {
        while (i > 0 && elements[i] > elements[parent(i)]) {
            warn() << *this;
            std::swap(elements[i], elements[parent(i)]); i = parent(i);
        }
    }
public:
    typedef PriorityQueue<T> This;

    PriorityQueue () {}
    PriorityQueue (const This& other) : elements(other.elements) {}
    This& operator= (const This& other) { return elements = other.elements, *this; }
    PriorityQueue (This&& other) : elements(std::move(other.elements)) {}
    This& operator= (This&& other) { return elements = std::move(other.elements), *this; }
    ~PriorityQueue () {}

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
            } else if (left(i) >= n) {
                warn() << *this;
                std::swap(elements[i], elements[right(i)]); i = right(i);
            } else if (right(i) >= n) {
                warn() << *this;
                std::swap(elements[i], elements[left(i)]); i = left(i);
            } else if (elements[left(i)] >= elements[right(i)]) {
                warn() << *this;
                std::swap(elements[i], elements[left(i)]); i = left(i);
            } else {
                warn() << *this;
                std::swap(elements[i], elements[right(i)]); i = right(i);
            }
        }
        if (i < n - 1) {
            warn() << *this;
            warn() << elements[n - 1];
            std::swap(elements[i], elements[n - 1]);
            heapify(elements[i]);
        }
        warn() << *this;
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
