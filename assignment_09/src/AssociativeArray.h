
#ifndef AssociativeArray_h
#define AssociativeArray_h
#include <algorithm>        // std::lower_bound
#include "DynamicArray.h"
#include "Queue.h"

struct AADefaultStrategy {
    template <typename KV, typename Key>
    static size_t locate (
        const DynamicArray<KV>& elements,
        const Key& key,
        size_t n
    ) {
        for (size_t i = 0; i < n; ++i) {
            if (elements[i].first == key) {
                return i;
            }
        }
        return n;
    }
    template <typename KV>
    static void insert (
        DynamicArray<KV>& elements,
        const KV& kv,
        size_t  i,
        size_t& n
    ) {
        elements[i] = kv;
        if (i == n) { ++n; }
    }
};


struct AAFastBinaryStrategy {
    template <typename KV, typename Key>
    static size_t locate (
        const DynamicArray<KV>& elements,
        const Key& key,
        size_t n
    ) {
        const KV* end    = &(elements[n]);
        const KV* begin  = &(elements[0]);
        auto ptr   = std::upper_bound(begin, end, KV(key, {}), [](const KV& a, const KV& b) { return a.first < b.first; });
        return static_cast<size_t>(ptr - begin);
    }
    template <typename KV>
    static void insert (
        DynamicArray<KV>& elements,
        const KV& kv,
        size_t  i,
        size_t& n
    ) {
        elements[i] = kv;
        if (i == n) { ++n; }
    }
};

/* Unordered associative array */
template <typename Key, typename Value, typename InsertStrategy = AADefaultStrategy>
class AssociativeArray {
    typedef AssociativeArray<Key,Value,InsertStrategy>  This;
    typedef std::pair<Key,Value>                        KV;

    DynamicArray<KV>    elements;
    size_t              count = 0;        // num unique elements
    const Value         defaultValue;
public:
    AssociativeArray () : defaultValue() {}
    AssociativeArray (const std::initializer_list<KV>& values) { insert(values); }
    AssociativeArray (const This& other) : elements(other.elements), count(other.count), defaultValue() {}
    This& operator=  (const This& other) { elements = other.elements; count = other.count; return *this; }
    ~AssociativeArray () { clear(); }

    // Move operations
    AssociativeArray (This&& other) : AssociativeArray() { *this = std::move(other); }
    This& operator=  (This&& other) { std::swap(elements, other.elements); std::swap(count, other.count); return *this; }
private:
    size_t find (const Key& key) const { return InsertStrategy::locate(elements, key, count); }
    void insert (size_t i, const KV& kv) { InsertStrategy::insert(elements, kv, i, count); }
public:
    size_t size () const { return count; }
    operator bool () const { return count != 0; }
    void  clear () { count = 0; }

    Value& operator[] (const Key& key) {
        auto i = find(key);
        if (i == count) {
            insert(i, { key, {}});
        }
        return elements[i].second;
    }
    const Value& operator[] (const Key& key) const {
        auto i = find(key);
        if (i == count) { return elements[i].second; }
        else            { return defaultValue; }
    }
    bool containsKey (const Key& key) {
        return find(key) != count;
    }
    void deleteKey (const Key& key) {
        auto i = find(key);
        if (i != count) {
            std::swap(elements[i], elements[--count]);
        }
    }
    void insert (const std::initializer_list<KV>& values) {
        insert(values.begin(), values.end());
    }
    void insert (const KV& kv) { 
        insert(find(kv.first), kv); 
    }
    template <typename It>
    void insert (It begin, It end) {
        for (; begin != end; ++begin) {
            insert(*begin);
        }
    }

    // Required as part of assignment spec, but not used (is super inefficient,
    // creating tons of needless allocations, particularly as my Queue implementation
    // is NOT very memory efficient (impl as linked list for simplicty)).
    Queue<Key> keys () const {
        Queue<Key> keys;
        for (const auto& kv : *this) {
            keys.push(kv.first);
        }
        return keys;
    }

    // Iterator implementation.
    // Is very fast / efficient as it uses raw pointers, possible b/c keys / values are stored in
    // contiguous memory within a dynamic array. Needless to say, using iterators while modifying the
    // array (ie. inserting elements), is absolutely illegal; using eg. mismatched iterators 
    // that refer to different memory locations is completely undefined and may crash your program.
    //
    // TLDR; these itererators may be used for:
    //  – iteration
    //  - sorting
    //  - item removal via eg. std::remove_if
    // They may NOT be used for:
    //  - insertion
    //  - any algorithm that grows the array, etc (they're just raw pointers so that wouldn't work)
    //
    // Also, note that we call end() in begin() (then throw away the result), which is to
    // explicitely grow the array to whatever count is. This is an edge case that can occur
    // in some situations.
    //
    typedef KV* iterator;
    typedef const KV* const_iterator;

    iterator begin () { auto reserve = end(); return &elements[0]; }
    iterator end   () { return &elements[count]; }
    const_iterator begin () const { auto reserve = end(); return &elements[0]; }
    const_iterator end   () const { return &elements[count]; }
};

#endif // AssociativeArray_h
