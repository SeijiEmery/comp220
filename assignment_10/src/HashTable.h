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

        Bitset (size_t* data, size_t size) { assign(data, size); }
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

    // Storage abstraction, manages memory for the HashTable type
    // somewhat... complicated (probably more than it has to be), as I wanted the bitset + elements array to be in contiguous memory >.>
    // Basically, this is a static (non-growing) sparse key-value array that the HashTable indexes into.
    //
    // fun features:
    //  – memory is contiguous (ie. data array is referenced by both bitset + elements)
    //  - only one allocation / free per storage instance
    //  - custom (very manual) memory management; data layout is sparse (ie. not all elements have been initialized)
    //  - lookup via contains()
    //  - insertion via maybeInsert() – supports Key or KeyValue
    //  - deletion  via maybeDelete()
    //  - iterator impl that skips over empty values
    //  - an additional, direct iteration algorithm (each()) that iterates over both; necessary
    //    to implement some algorithms without exposing Storage internals
    //  - class is neatly encapsulated w/ data hiding (including from the outer HashTable impl; can use the public inteface only)
    //  - no default ctor; disabled copies
    //  - does have a move ctor, since that was sorta necessary...
    //
    class Storage {
        size_t      capacity;
        uint8_t*    data;
        Bitset      bitset;
        KeyValue*   elements;
    public:
        Storage (size_t capacity)
            : capacity(capacity)
            , data(new uint8_t[ Bitset::allocationSize(capacity) + capacity * sizeof(KeyValue) ])
            , bitset(reinterpret_cast<size_t*>(data), Bitset::allocationSize(capacity))
            , elements(reinterpret_cast<KeyValue*>(&data[Bitset::allocationSize(capacity) * sizeof(size_t)]))
        {}
        Storage (const Storage& other) = delete;
        Storage& operator= (const Storage& other) = delete;
        Storage (Storage&& other) 
            : Storage(other.size())
        { *this = std::move(other); }
        Storage& operator= (Storage&& other) {
            std::swap(capacity, other.capacity);
            std::swap(data, other.data);
            std::swap(bitset, other.bitset);
            std::swap(elements, other.elements);
            return *this;
        }
        void swap (Storage& other) {
            std::swap(capacity, other.capacity);
            std::swap(data, other.data);
            std::swap(bitset, other.bitset);
            std::swap(elements, other.elements);
        }
        ~Storage () {
            for (size_t i = 0; i < size(); ++i) {
                maybeDelete(i);
            }
            delete[] data;
        }
        friend std::ostream& operator<< (std::ostream& os, const Storage& self) {
            return os << "capacity = " << self.size() << ", bitset " << self.bitset;
        }
        size_t size () const { return capacity; }
        void clear () { bitset.clear(); }

        bool contains (size_t index) const { return bitset.get(index); }
        bool maybeInsert (size_t index, const Key& key) {
            if (!contains(index)) {
                bitset.set(index);
                // call constructor -- necessary b/c this is unitialized memory and assignment alone does not work for all types
                new (&elements[index]) KeyValue (key, {});
                return true;
            }
            return false;
        }
        bool maybeInsert (size_t index, const KeyValue& kv) {
            if (!contains(index)) {
                bitset.set(index);
                // call constructor -- necessary b/c this is unitialized memory and assignment alone does not work for all types
                new (&elements[index]) KeyValue(kv);
                return true;
            }
            return false;
        }
        bool maybeDelete (size_t index) {
            if (contains(index)) {
                bitset.clear(index);
                // call destructor -- necessary b/c we're manually managing memory, and 
                // have sparse value initialization (for uh, performance reasons >_<). 
                // Slightly silly, but has been thoroughly tested and is guaranteed to work (I think...).
                elements[index].~KeyValue();
                return true;
            }
            return false;
        }

        KeyValue& operator[] (size_t index) { return elements[index]; }
        const KeyValue& operator[] (size_t index) const { return elements[index]; }

        template <typename F>
        void each (F f) {
            for (size_t i = 0; i < size(); ++i) {
                f(i, bitset.get(i), elements[i]);
            }
        }

        template <typename V>
        class Iterator {
            friend class Storage;
            Storage*    storage;
            size_t      index;

            void advance () { while(index < storage->size() && !storage->bitset.get(index)) ++index; }
            Iterator (Storage* storage, size_t index) : storage(storage), index(index) { advance(); }
        public:
            Iterator (const Iterator& other) = default;
            Iterator& operator= (const Iterator& other) = default;

            template <typename U> bool operator== (const Iterator<U>& other) const { return storage == other.storage && index == other.index; }
            template <typename U> bool operator!= (const Iterator<U>& other) const { return storage != other.storage || index != other.index; }

            Iterator& operator++ () { if (index < storage->size()) { ++index; advance(); } return *this; }
            V& operator*  () const { return storage->elements[index]; }
            V* operator-> () const { return &storage->elements[index]; }

            friend std::ostream& operator<< (std::ostream& os, const Iterator<V>& it) {
                return os << "HashTable::Iterator {" << *it.storage << ", index " << it.index << " }";
            }
            operator Iterator<const V> () const { return { storage, index }; }
        };
        typedef Iterator<KeyValue>       iterator;
        typedef Iterator<const KeyValue> const_iterator;

        iterator       make_iterator (size_t index) { return { this, index }; }
        const_iterator make_iterator (size_t index) const { return { const_cast<Storage*>(this), index }; }

        iterator       begin () { return { this, 0 }; }
        iterator       end   () { return { this, size() }; }
        const_iterator begin () const { return { const_cast<Storage*>(this), 0 }; }
        const_iterator end   () const { return { const_cast<Storage*>(this), size() }; }
        const_iterator cbegin () const { return begin(); }
        const_iterator cend  ()  const { return end(); }
    };

    HashFunction hashFunction;
    Storage      storage;
    size_t       capacityThreshold;
    size_t       count = 0;

    const char* color;
    LineWriter info () const { return writeln(std::cout, color); }
public:
    //
    // Utility methods...
    //
    static Storage make_storage (size_t capacity) {
        return { capacity };
    }
    This create (size_t capacity, double threshold = 0.8) const {
        return { hashFunction, capacity, threshold };
    }
    This clone () { return *this; }
    friend std::ostream& operator<< (std::ostream& os, const This& self) {
        return os << "HashTable { size = " << self.size() << ", " << self.storage << " }";
    }

    //
    // Primary interface...
    //
    HashTable () = delete;
    HashTable (HashFunction hashFunction, size_t capacity = 0, double threshold = 0.8)
        : hashFunction(hashFunction)
        , storage(capacity)
        , capacityThreshold((size_t)(capacity * threshold))
        , color(getCyclingColor())
    {}
    HashTable (const This& other)
        : hashFunction(other.hashFunction)
        , storage(other.storage.size())
        , capacityThreshold(other.capacityThreshold)
        , color(getCyclingColor())
    {
        insert(other.begin(), other.end());   
    }
    This& operator= (const This& other) {
        clear();
        insert(other.begin(), other.end());
        return *this;
    }
    void swap (This& other) {
        std::swap(hashFunction, other.hashFunction);
        storage.swap(other.storage);
        std::swap(capacityThreshold, other.capacityThreshold);
        std::swap(count, other.count);
        std::swap(color, other.color);
    }

    // This& operator= (This&& other) {
    //     std::swap(hashFunction, other.hashFunction);
    //     std::swap(storage, other.storage);
    //     std::swap(capacityThreshold, other.capacityThreshold);
    //     std::swap(count, other.count);
    //     std::swap(color, other.color);
    //     return *this;
    // }
    // HashTable (This&& other)
    //     : hashFunction(std::move(other.hashFunction))
    //     , storage(std::move(other.storage))
    //     , capacityThreshold(other.capacityThreshold)
    //     , color(getCyclingColor())
    // {}
    ~HashTable () {}

    size_t size () const { return count; }
    void resize (size_t size) {
        info() << "Resizing " << storage.size() << " => " << size << " (has " << count << " elements)";
        assert((size > count) && "Not enough capacity to hold all data elements!");
        HashTable temp(hashFunction, size);
        temp.insert(begin(), end());
        swap(temp);
    }
    void debugMemLayout () {
        storage.each([&](size_t i, bool set, const KeyValue& kv) {
            if (set) {
                info() << "    " << i << ": " << kv.first << " => " << kv.second;
            } else {
                info() << "    " << i << ": --";
            }
        });
    }
    // Reinsert all elements
    void reinsert () { 
        resize(storage.capacity()); 
    }
    // Clear all elements
    void clear () {
        if (count != 0) {
            info() << "Clearing " << storage.size();
            count = 0;
            storage.clear();
        } else {
            info() << "Already cleared " << storage.size();
        }
    }
private:
    size_t locate (const Key& key) const {
        size_t hash = hashFunction(key) % storage.size(); size_t iterations = storage.size();
        while (storage.contains(hash) && storage[hash].first != key) {
            hash = (hash + 1) % storage.size();
            assert(iterations --> 0 && "No storage remaining, cannot locate / insert element!");
        }
        info() << "Found hash for key " << key << ": " << hash << ", distance " << iterations << ")";
        return hash;
    }
public:
    const Value& operator[] (const Key& key) const {
        return storage[locate(key)].second;
    }
    Value& operator[] (const Key& key) {
        auto index = locate(key);
        if (storage.maybeInsert(index, key)) {
            info() << "Inserted " << key << " at " << index;
            if (++count > capacityThreshold) {
                info() << "Capacity exceeded, resizing: " << count << " > " << capacityThreshold;
                resize(storage.size() * 2);
            }
        }
        return storage[index].second;
    }
    void insert (const KeyValue& kv) {
        auto index = locate(kv.first);
        if (storage.maybeInsert(index, kv)) {
            info() << "inserted " << kv.first << ", " << kv.second << " at " << index;
            if (++count > capacityThreshold) {
                info() << "Capacity exceeded, resizing: " << count << " > " << capacityThreshold;
                resize(storage.size() * 2);
            }
        }
    }
    bool containsKey (const Key& key) {
        return storage.contains(locate(key));
    }
    void deleteKey (const Key& key) {
        auto index = locate(key);
        if (storage.maybeDelete(index)) {
            info() << "deleted " << key << " at " << index;
            --count;
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

    typedef typename Storage::iterator iterator;
    typedef typename Storage::const_iterator const_iterator;

    iterator begin () { return storage.begin(); }
    iterator end   () { return storage.end();   }
    const_iterator begin () const { return storage.begin(); }
    const_iterator end   () const { return storage.end();   }
    const_iterator cbegin () const { return begin(); }
    const_iterator cend  ()  const { return end(); }

    iterator find (const Key& key) { 
        auto index = locate(key);
        return storage.contains(index) ?
            storage.make_iterator(index) :
            storage.end();
    }
    const_iterator find (const Key& key) const {
        auto index = locate(key);
        return storage.contains(index) ?
            storage.make_iterator(index) :
            storage.end();
    }
};

template <typename Key, typename Value, typename HashFunction>
auto make_hashtable (HashFunction hashFunction, size_t size = 0) -> HashTable<Key,Value,HashFunction> {
    return { hashFunction, size };
}



#endif // HashTable_h
