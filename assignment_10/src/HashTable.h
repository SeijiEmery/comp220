#ifndef HashTable_h
#define HashTable_h

#include <utility>      // std::pair
#include <algorithm>    // std::fill
#include <cassert>      // assert

#include <iostream>

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
LineWriter report () { return writeln(SET_CYAN); }
LineWriter warn   () { return writeln(SET_RED); }
LineWriter info   () { return writeln(SET_GREEN); }


const static char* colors[11] = { SET_COLOR("31"), SET_COLOR("32"), SET_COLOR("33"), SET_COLOR("34"), SET_COLOR("35"), SET_COLOR("1;30"), SET_COLOR("1;31"), SET_COLOR("1;32"), SET_COLOR("1;33"), SET_COLOR("1;34"), SET_COLOR("1;35") };
static int nextColor = 0;
const char* getCyclingColor () { auto color = colors[nextColor]; ++nextColor; nextColor %= (sizeof(colors) / sizeof(colors[0])); return color; }


template <typename Key, typename Value, typename HashFunction = size_t(*)(const Key&)>
class HashTable {
public:
    typedef HashTable<Key, Value, HashFunction> This;
    typedef std::pair<Key, Value>               KeyValue;
private:
    // Can't use DynamicArray (or, hence, my Bitset impl)
    // Note: this is really stupid, b/c we have to reimplement everything from scratch -_-
    // Note: this is likely to INCREASE bugs, b/c the previous code was well tested...

    // Non-owning bitset
    struct Bitset {
        size_t  size = 0;
        size_t* data = nullptr;

        Bitset () {}
        Bitset (const Bitset& other) = delete;
        Bitset& operator= (const Bitset& other) = delete;
        Bitset (Bitset&& other) { *this = std::move(other); }
        Bitset& operator= (Bitset&& other) { return std::swap(data, other.data), std::swap(size, other.size), *this; }

        static size_t allocationSize (size_t count) {
            return (count + (sizeof(size_t) * 8) - 1) / (sizeof(size_t) * 8);
        }
        void clear () { if (data) { std::fill(&data[0], &data[size], 0); } }
        void assign (size_t* data, size_t size) {
            this->data = data;
            this->size = size;
            assert((data == nullptr) == (size == 0));
            clear();
        }
        operator bool () const { return data != nullptr; }
        bool get   (size_t index) const { return data[index / sizeof(size_t)] & (1 << (index % sizeof(size_t))); }
        void set   (size_t index) const { data[index / sizeof(size_t)] |= (1 << (index % sizeof(size_t))); }
        void clear (size_t index) const { data[index / sizeof(size_t)] &= ~(1 << (index % sizeof(size_t))); }

        friend std::ostream& operator<< (std::ostream& os, const Bitset& bitset) {
            os << bitset.size << ":";
            for (size_t i = 0; i < bitset.size; ++i) {
                for (size_t mask = 1; mask != 0; mask <<= 1) {
                    os << ((bitset.data[i] & mask) ? '1' : '0');
                }
            }
            return os;
        }
    };

    HashFunction hashFunction;
    size_t      capacity = 0;           // # max key / value pairs
    size_t      capacityThreshold = 0;  // 
    size_t      count = 0;              // # non-empty key / value pairs
    size_t*     data = nullptr;         // owning pointer for all data elements
    Bitset      bitset;                 // points into data
    KeyValue*   elements = nullptr;     // points into data

    const char* color;
    LineWriter info () const { return writeln(std::cout, color); }
public:
    friend std::ostream& operator<< (std::ostream& os, const This& self) {
        return os << "HashTable { capacity = " << self.capacity 
            << ", size = " << self.count 
            << ", bitset = " << self.bitset << " }"; 
    }


    HashTable () = delete;
    HashTable (HashFunction hashFunction, size_t capacity = 0)
        : hashFunction(hashFunction)
        , capacity(capacity = capacity ? capacity : 128)
        , capacityThreshold((size_t)(capacity * 0.75))
        , color(getCyclingColor())
    {
        info() << "Allocated " << capacity;
        assert(capacity != 0);
        size_t bitsetSize = Bitset::allocationSize(capacity);
        size_t elementSize = (sizeof(KeyValue) * capacity + sizeof(size_t) - 1) / sizeof(size_t);

        info() << "bitset size: " << bitsetSize;
        info() << "element size: " << elementSize 
            << " (" << capacity << " * " << sizeof(KeyValue) << " / " << sizeof(size_t) << ")";

        data = new size_t[bitsetSize + elementSize];
        info() << "allocated pointer " << data << ", capacity " << capacity 
               << ", " << (bitsetSize + elementSize) * sizeof(size_t) << " bytes";
        data[bitsetSize + elementSize - 1] = 0;

        bitset.assign(data, bitsetSize);
        elements = reinterpret_cast<KeyValue*>(&data[bitsetSize]);
        info() << "data ptr " << data;
        info() << "elem ptr " << elements;
        info() << "back ptr " << &elements[capacity];

        info() << "elements offset: " << reinterpret_cast<size_t>(elements) - reinterpret_cast<size_t>(data);
        info() << "back offset: " << reinterpret_cast<size_t>(&elements[capacity-1]) - reinterpret_cast<size_t>(data);
        elements[capacity-1] = {};
    }
    HashTable (const This& other) 
        : HashTable(other.hashFunction, other.capacity)
    { 
        info() << "COPY CTOR: copying " << other.count << " elements"; 
        assert(data != nullptr);
        insert(other.begin(), other.end()); 
    }
    This& operator= (const This& other) { 
        info() << "COPY ASSIGNMENT";
        HashTable temp(other); 
        std::swap(*this, temp); 
        return *this; 
    }
    HashTable (This&& other)
        : HashTable(other.hashFunction, other.capacity)
    { 
        info() << "MOVE CTOR: swapping " << other.data << ", " << data << " (capacity " << other.capacity << ", count " << other.count << ")";
        assert(data != nullptr);
        *this = std::move(other); 
    }
    This& operator= (This&& other) {
        info() << "MOVE ASSIGNMENT";
        info() << "Swapping " << capacity;
        other.info() << "with " << other.capacity;

        std::swap(hashFunction, other.hashFunction);
        std::swap(capacity, other.capacity);
        std::swap(capacityThreshold, other.capacityThreshold);
        std::swap(count, other.count);
        std::swap(data, other.data);
        std::swap(bitset, other.bitset);
        std::swap(elements, other.elements);
        std::swap(color, other.color);
        return *this;
    }
    ~HashTable () { 
        info() << "freeing pointer " << data << ", capacity " << capacity;
        assert(data != nullptr);
        // info() << "Destructing " << capacity;
        info() << "calling destructor(s) (size " << count << ")";
        clear();
        delete[] data;
    }

    size_t size () const { return count; }
    void resize (size_t size) {
        info() << "Resizing " << capacity << " => " << size << " (has " << count << " elements)";
        assert((size > count) && "Not enough capacity to hold all data elements!");
        HashTable temp(hashFunction, size);
        temp.insert(begin(), end());
        *this = std::move(temp);
    }
    void debugMemLayout () {
        for (size_t i = 0; i < capacity; ++i) {
            if (bitset.get(i)) {
                info() << "    " << i << ": " << elements[i].first << " => " << elements[i].second;
            } else {
                info() << "    " << i << ": --";
            }
        }
    }
    // Reinsert all elements
    void reinsert () { 
        resize(capacity); 
    }
    // Clear all elements
    void clear () {
        if (count != 0) {
            info() << "Clearing " << capacity;

            // Fire destructors on all non-empty elements
            for (auto& kv : *this) {
                kv.first.~Key();
                kv.second.~Value();
            }
            // Clear size + usage bitmask
            count = 0;
            bitset.clear();
        } else {
            info() << "Already cleared " << capacity;
        }
    }
private:
    size_t locate (const Key& key) const {
        size_t hash = hashFunction(key) % capacity; size_t iterations = 0;
        while (bitset.get(hash) && elements[hash].first != key) {
            hash = (hash + 1) % capacity;
            ++iterations;
        }
        info() << "Found hash for key " << key << ": " << hash << ", distance " << iterations << " (capacity " << capacity << ")";
        return hash;
    }
public:
    const Value& operator[] (const Key& key) const {
        return elements[locate(key)].second;
    }
    Value& operator[] (const Key& key) {
        auto index = locate(key);
        if (!bitset.get(index)) {
            info() << "inserting " << key << " at " << index << " into " << elements;
            ++count;
            bitset.set(index);
            elements[index] = { key, {} };
            if (count > capacityThreshold) {
                info() << "Capacity exceeded: " << count << " > " << capacityThreshold;
                resize(capacity * 2);
            }
        }
        return elements[index].second;
    }
    void insert (const KeyValue& kv) {
        auto index = locate(kv.first);
        elements[index] = kv;
        if (!bitset.get(index)) {
            info() << "inserting " << kv.first << " at " << index << " into " << elements;
            ++count;
            bitset.set(index);
            if (count > capacityThreshold) {
                info() << "Capacity exceeded: " << count << " > " << capacityThreshold;
                resize(capacity * 2);
            }
        }
    }
    bool containsKey (const Key& key) {
        return bitset.get(locate(key));
    }
    void deleteKey (const Key& key) {
        auto index = locate(key);
        if (bitset.get(index)) {
            info() << "deleting " << key << " at " << index << " from " << elements;
            --count;
            bitset.clear(index);
            elements[index].~KeyValue();
        }
    }
    void insert (const Key& key, const Value& value) {
        insert({ key, value });
    }
    template <typename It>
    void insert (It begin, It end) {
        info() << "Inserting values";
        for (; begin != end; ++begin) {
            insert(*begin);
        }
    }
private:
    template <typename T>
    class Iterator {
        friend class HashTable;
        HashTable<Key,Value,HashFunction>& hashtable;
        size_t  index = 0;

        LineWriter info () const { return hashtable.info(); }

        Iterator (decltype(hashtable)& hashtable, size_t index = 0)
            : hashtable(hashtable), index(index) 
        {
            info() << "Constructed iterator: " << index;
        }
        void advanceToNextNonEmptyElement () {
            auto start = index;
            while (index < hashtable.capacity && !hashtable.bitset.get(index)) {
                ++index;
            }
            if (start != index) {
                info() << "Advanced index " << start << " => " << index;
            }
        }

    public:
        Iterator (const Iterator<T>& other) : hashtable(other.hashtable), index(other.index) {
            info() << "Copying iterator";
            advanceToNextNonEmptyElement();
        }
        Iterator& operator= (const Iterator<T>& other) { hashtable = other.hashtable; index = other.index; return *this; }
        operator bool () const { return index < hashtable.capacity; }

        template <typename T2> bool operator== (const Iterator<T2>& other) const { return &hashtable == &(other.hashtable) && index == other.index; } 
        template <typename T2> bool operator!= (const Iterator<T2>& other) const { return &hashtable != &(other.hashtable) || index != other.index; }

        Iterator<T>& operator++ () {
            if (index < hashtable.capacity) {
                ++index;
                advanceToNextNonEmptyElement();
            } else {
                info() << "Hit end: " << index;
            }
            return *this; 
        }
        T& operator*  () const { info() << "Fetching value at index " << index << ": " << hashtable.elements[index].first << ", " << hashtable.elements[index].second; return hashtable.elements[index]; }
        T* operator-> () const { info() << "Fetching value at index " << index << ": " << hashtable.elements[index].first << ", " << hashtable.elements[index].second; return hashtable.elements[index]; }
    };
public:
    typedef Iterator<KeyValue> iterator;
    typedef Iterator<const KeyValue> const_iterator;

    iterator begin () { info() << "begin()"; return iterator(*this, 0); }
    iterator end   () { info() << "end()"; return iterator(*this, capacity); }
    const_iterator begin () const { info() << "cbegin()"; return const_iterator(const_cast<This&>(*this), 0); }
    const_iterator end   () const { info() << "cend()";   return const_iterator(const_cast<This&>(*this), capacity); }

    iterator find (const Key& key) {
        auto index = locate(key);
        return bitset.get(index) ?
            iterator(*this, index) :
            end();
    }
    const_iterator find (const Key& key) const {
        auto index = locate(key);
        return bitset.get(index) ?
            const_iterator(*this, index) :
            end();
    }
};

template <typename Key, typename Value, typename HashFunction>
auto make_hashtable (HashFunction hashFunction, size_t size = 0) -> HashTable<Key,Value,HashFunction> {
    return { hashFunction, size };
}



#endif // HashTable_h
