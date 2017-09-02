
template <typename Pointer>
struct TSlice {
    Pointer ptr;
    size_t count  = 0;
    size_t start  = 0;
    int    step   = 1;
public:
    TSlice () : ptr(nullptr) {}
    TSlice (Pointer ptr, size_t count, size_t start = 0, int step = 1)
        : ptr(ptr), count(count), start(start), step(step) {}
    TSlice (Pointer begin, Pointer end, int step = 1)
        : TSlice(begin, static_cast<size_t>(std::max(0, std::distance(begin, end), step))) {}
    TSlice (const TSlice& slice) : TSlice() { operator=(slice); }
    TSlice (TSlice& slice) : TSlice() { operator=(slice); }

    TSlice& operator= (TSlice& slice) {
        std::swap(slice.ptr, ptr);
        std::swap(slice.count, count);
        std::swap(slice.start, start);
        std::swap(slice.step, step);
        return *this;
    }
    TSlice& operator= (const TSlice& slice) {
        ptr = slice.ptr;
        count = slice.count;
        start = slice.start;
        step = slice.step;
        return *this;
    }

public:
    size_t size () const { return count; }
    size_t start_index () const { return start; }
    size_t end_index   () const { return (size_t)( (int)start + (int)count * step )) ); }
    int    step_size   () const { return step; }

protected:
    int get_real_index (int i) {
        if (i < 0) { i += count; }
        return start + i * step;
    }
    bool check_bounds (int real_index) {
        return real_index >= 0 && real_index < end_index();
    }
    size_t get_checked_index (int i) {
        auto index = get_real_index();
        assert(check_bounds(index));
        return (size_t)index;
    }
public:
    auto& operator[] (int i)             { return ptr[get_checked_index(i)]; }
    const auto& operator[] (int i) const { return ptr[get_checked_index(i)]; }

    auto& operator[] (size_t i)             { return operator[]((int)i); }
    const auto& operator[] (size_t i) const { return operator[]((int)i); }

    TSlice<T> slice (int start = 0, int end = -1, int step = 1) {
        start = get_real_index(start); assert(start >= start_index() && start <= end_index());
        end   = get_real_index(end);   assert(end >= start_index() && end <= end_index());
        step *= this->step;
        if (start < end) { 
            std::swap(start, end); 
        }
        size_t count = (size_t)(end - start);
        assert(count % step == 0);
        return { ptr, count / step, step };
    }

    Iterator begin () { return { this, 0 }; }
    Iterator end   () { return { this, count }; }

    Iterator cbegin () { return begin(); }
    Iterator cend   () { return end(); }

    friend Iterator;
    class Iterator {
        TSlice<T>* slice = nullptr;
        int i = 0;
    public:
        Iterator () {}
        Iterator (decltype(slice) slice, int i) : slice(slice), i(i) {}
        Iterator (const Iterator&) = default;
        Iterator& operator= (const Iterator&) = default;

        Iterator& operator++ () { ++i; return *this; }
        Iterator& operator-- () { --i; return *this; }

        Iterator& operator+= (int n) { i += n; return *this; }
        Iterator& operator-= (int n) { i += n; return *this; }

        operator bool () { return slice != nullptr && slice->is_bounded(i); }

        friend bool operator== (cosnt Iterator& a, const Iterator& b) {
            if (a.slice != b.slice) return false;
            if (!a && !b) return true;
            return a.i == b.i;
        }
        friend bool operator!= (const Iterator& a, const Iterator& b) {
            return !(a == b);
        }
        friend int operator- (const Iterator& a, const Iterator& b) {
            assert(a == b); return a.i - b.i;
        }
        friend bool operator> (const Iterator& a, const Iterator& b) { return a - b > 0; }
        friend bool operator< (const Iterator& a, const Iterator& b) { return a - b < 0; }
        friend bool operator>= (const Iterator& a, const Iterator& b) { return a - b >= 0; }
        friend bool operator<= (const Iterator& a, const Iterator& b) { return a - b <= 0; }

        auto& operator*  () { assert(slice); return (*slice)[index]; }
        auto& operator-> () { assert(slice); return (*slice)[index]; }
        auto& operator[] (int n) { assert(slice); return (*slice)[index]; }
    }
};

#ifdef UNIT_TESTS_ENABLED



#endif







template <typename T, size_t N, size_t M>
class Matrix {
    constexpr size_t SIZE = N * M;
    T[SIZE] _data;
public:

    Matrix () : _data(0) {}
    Matrix (std::initializer_list<T> values) : _data(values) {}
    Matrix (const Matrix<N, M>& mat) : _data(mat._data) {}

    T* begin () { return &_data[0]; }
    T* end   () { return &_data[SIZE]; }

    const T* cbegin () { return &data[0]; }
    const T* cend   () { return &data[SIZE]; }

    Slice<T*> row (size_t n) { assert(n < N); return { &_data, N, n * M, 1 }; }
    Slice<T*> col (size_t m) { assert(m < M); return { &_data, M, m,     N }; }
};
