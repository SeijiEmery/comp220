#ifndef HashTable_h
#define HashTable_h

#include <utility>      // std::pair
#include <algorithm>    // std::fill
#include <cassert>      // assert

#include <iostream>


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
        friend class Storage;
        typedef uint8_t word_t;
        static constexpr size_t N = (sizeof(word_t) * 8);
        size_t  size = 0;
        word_t* data = nullptr;

        Bitset (word_t* data, size_t size) { assign(data, size); }
        Bitset (const Bitset& other) = delete;
        Bitset& operator= (const Bitset& other) = delete;
        Bitset (Bitset&& other) { *this = std::move(other); }
        Bitset& operator= (Bitset&& other) { return std::swap(data, other.data), std::swap(size, other.size), *this; }

        static size_t allocationSize (size_t count) {
            return (count + N - 1) / N;
        }
        void clear () { 
            assert(data != nullptr);
            std::fill(&data[0], &data[size], 0); 
        }
        void assign (word_t* data, size_t size) {
            this->data = data;
            this->size = size;
            assert((data == nullptr) == (size == 0));
            clear();
        }
        operator bool () const { return data != nullptr; }
        bool get   (size_t index) const { return data[index / N] & (1 << (index % N)); }
        void set   (size_t index) const { data[index / N] |= (1 << (index % N)); }
        void clear (size_t index) const { data[index / N] &= (~(1 << (index % N))); }

        friend std::ostream& operator<< (std::ostream& os, const Bitset& bitset) {
            os << bitset.size << ":";
            for (size_t i = 0; i < bitset.size; ++i) {
                for (word_t mask = 1; mask != 0; mask <<= 1) {
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
        void*       data;
        Bitset      bitset;
        KeyValue*   elements;
    public:
        Storage (size_t capacity)
            : capacity(capacity)
            , data((void*)(new uint8_t[Bitset::allocationSize(capacity) + capacity * sizeof(KeyValue)]))
            , bitset(reinterpret_cast<typename Bitset::word_t*>(data), Bitset::allocationSize(capacity))
            , elements(reinterpret_cast<KeyValue*>(&bitset.data[bitset.size]))
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
            delete[] ((uint8_t*)data);
        }
        friend std::ostream& operator<< (std::ostream& os, const Storage& self) {
            return os << "capacity = " << self.size() << ", bitset " << self.bitset;
        }
        size_t size () const { return capacity; }
        void clear () { bitset.clear(); }

        bool contains (size_t index) const { return index < size() && bitset.get(index); }
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

            void advance () { 
                while(index < storage->size() && !storage->bitset.get(index)) {
                    ++index; 
                }
            }
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
                return os << "HashTable::Iterator {" << (it.index >= it.storage->size() ? "valid " : "invalid ") << it.storage << " " << *it.storage << ", index " << it.index << " }";
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
    double       _loadFactor;
    size_t       capacityThreshold;
    size_t       count = 0;
    size_t       numCollisions = 0;     // statistics: # collisions, and total distance traveled
    size_t       collisionDist = 0;     // in collisions. updated by insert, delete, clear operations
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
        return os << "HashTable { size = " << self.size() 
            << ", collision-rate " << (self.numCollisions ? ((double)self.numCollisions / (double)self.capacity()) : 0)
            << ", collision-dist " << (self.numCollisions ? ((double)self.collisionDist / (double)self.numCollisions) : 0)
            << ", " << self.storage << " }";
    }

    //
    // Primary interface...
    //
    HashTable () = delete;
    HashTable (HashFunction hashFunction, size_t capacity = 0, double loadFactor = 0.8)
        : hashFunction(hashFunction)
        , storage(capacity)
        , _loadFactor(loadFactor)
        , capacityThreshold((size_t)(capacity * loadFactor))
    {}
    HashTable (const This& other)
        : hashFunction(other.hashFunction)
        , storage(other.capacity())
        , _loadFactor(other.loadFactor())
        , capacityThreshold(other.capacityThreshold)
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
        std::swap(_loadFactor, other._loadFactor);
        std::swap(capacityThreshold, other.capacityThreshold);
        std::swap(count, other.count);
        std::swap(numCollisions, other.numCollisions);
        std::swap(collisionDist, other.collisionDist);
    }
    ~HashTable () {}

    size_t size () const { return count; }
    size_t capacity () const { return storage.size(); }
    double loadFactor () const { return _loadFactor; }
    void   loadFactor (double lf) {
        if (lf < 0.1)   lf = 0.1;
        if (lf > 0.99)  lf = 0.99;
        if (lf != _loadFactor) {
            _loadFactor = lf;
            capacityThreshold = (size_t)(capacity() * loadFactor());
            resize(1);
        }
    }
    operator bool () const { return size() != 0; }

    void resize (size_t size) {
        if (size == 0) {
            size = 1;
        }
        // increase target size until it is large enough to fit all array elements w/out resizing
        while (this->size() + 1 >= size * loadFactor()) {
            size *= 2;
        }
        // Create new storage element w/ the target size, and swap it w/ our current storage
        Storage temp { size };
        storage.swap(temp);

        // Reset capacityThreshold to accomodate new storage size
        capacityThreshold = (size_t)(capacity() * loadFactor());
        assert(capacity() == size);
        assert(capacityThreshold > count);
        size_t prevSize = count;

        // clear count + reinsert
        count = 0;
        numCollisions = 0;
        collisionDist = 0;
        insert(temp.begin(), temp.end());
        assert(prevSize == count);
    }
    template <typename Callback>
    void each (Callback callback) {
        storage.each(callback);
    }
    // Reinsert all elements
    void reinsert () {
        resize(capacity());
    }
    // Clear all elements
    void clear () {
        if (count != 0) {
            // info() << "Clearing " << capacity();
            count = 0;
            numCollisions = 0;
            collisionDist = 0;
            storage.clear();
        } else {
            // info() << "Already cleared " << capacity();
        }
    }
private:
    size_t locate (const Key& key, size_t& iterations) const {
        size_t hash = hashFunction(key) % capacity();
        iterations = 0;
        while (storage.contains(hash) && storage[hash].first != key) {
            hash = (hash + 1) % capacity();
            if (++iterations >= capacity()) {
                // warn() << storage;
                // warn() << "WARNING: RECURSION LIMIT EXCEEEDED " 
                //     << "iterations = " << iterations
                //     << ", size = " << size()
                //     << ", capacity = " << capacity()
                //     << ", max = " << capacityThreshold;
                return capacity();
            }
        }
        return hash;
    }
    size_t locate (const Key& key) const {
        size_t _; return locate(key, _);
    }
public:
    const Value& operator[] (const Key& key) const {
        auto index = locate(key);
        if (index < capacity()) {
            return storage[index].second;
        } else {
            const static Value v = {};
            return v;
        }
    }
    Value& operator[] (const Key& key) {
        if (size() >= capacityThreshold) {
            resize(capacity() * 2);
        }
        assert(size() < capacityThreshold);

        size_t collisions;
        auto index = locate(key, collisions);
        assert(index < capacity());
        if (storage.maybeInsert(index, key)) {
            ++count;
            if (collisions) {
                numCollisions += 1;
                collisionDist += collisions;
            }
        }
        return storage[index].second;
    }
    bool insert (const KeyValue& kv) {
        if (size() >= capacityThreshold) {
            resize(capacity() * 2);
        }
        assert(size() < capacityThreshold);
        
        size_t collisions;
        auto index = locate(kv.first, collisions);
        assert(index < capacity());
        if (storage.maybeInsert(index, kv)) {
            ++count;
            if (collisions) {
                numCollisions += 1;
                collisionDist += collisions;
            }
            return true;
        } else {
            storage[index] = kv;
            return false;
        }
    }
    bool containsKey (const Key& key) {
        return storage.contains(locate(key));
    }
    void deleteKey (const Key& key) {
        size_t collisions;
        auto index = locate(key, collisions);
        if (storage.maybeDelete(index)) {
            --count;
            if (collisions) {
                numCollisions -= 1;
                collisionDist -= collisions;
            }
        }
    }
    bool insert (const Key& key, const Value& value) {
        return insert({ key, value });
    }
    template <typename It>
    void insert (It begin, It end) {
        for (; begin != end; ++begin) {
            insert(*begin);
        }
    }
    void insert (const std::initializer_list<KeyValue>& kvs) {
        insert(kvs.begin(), kvs.end());
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
auto make_hashtable (HashFunction hashFunction, size_t size = 1) -> HashTable<Key,Value,HashFunction> {
    return { hashFunction, size };
} 



#endif // HashTable_h
