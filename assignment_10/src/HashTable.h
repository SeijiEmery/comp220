#ifndef HashTable_h
#define HashTable_h

template <typename Key, typename Value>
class HashTable {
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
            return (count + sizeof(size_t) - 1) / sizeof(size_t);
        }
        void clear () { if (data) { std::fill(&data[0], &data[size / sizeof(size_t)], 0); } }
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
    };

    size_t      capacity = 0;           // # max key / value pairs
    size_t      count = 0;              // # non-empty key / value pairs
    size_t*     data = nullptr;         // owning pointer for all data elements
    Bitset      bitset;                 // points into data
    KeyValue*   elements = nullptr;     // points into data
public:
    typedef std::pair<Key, Value> KeyValue;

    HashTable () = delete;
    HashTable (size_t capacity) : capacity(capacity = capacity || 128) {
        assert(capacity != 0);
        size_t size = Bitset::allocationSize(capacity) + (sizeof(KeyValue) * capacity + sizeof(size_t) - 1) / sizeof(size_t);
        data = new size_t[size];
        bitset.assign(mem);
        elements = reinterpret_cast<KeyValue*>(&mem[Bitset::allocationSize(capacity)]);
    }
    HashTable (const This& other) { insert(other.begin(), other.end()); }
    This& operator= (const This& other) { HashTable temp(other); std::swap(*this, temp); return *this; }
    HashTable (This&& other) { *this = std::move(other); }
    This& operator= (This&& other) {
        std::swap(capacity, other.capacity);
        std::swap(count, other.count);
        std::swap(data, other.data);
        std::swap(bitset, other.bitset);
        std::swap(elements, other.elements);
    }
    ~HashTable () { clear(); delete[] data; }

    size_t size () const { return count; }
    void resize (size_t size) {
        assert((size > count) && "Not enough capacity to hold all data elements!");
        HashTable temp(size);
        temp.insert(begin(), end());
        *this = std::move(temp);
    }
    // Reinsert all elements
    void reinsert () { 
        resize(capacity); 
    }
    // Clear all elements
    void clear () {
        if (count != 0) {
            // Fire destructors on all non-empty elements
            for (auto& kv : *this) {
                kv.first.~Key();
                kv.second.~Value();
            }
            // Clear size + usage bitmask
            count = 0;
            bitset.clear();
        }
    }
private:
    size_t locate (const Key& key) const {
        size_t hash = hashFunction(key) % capacity;
        while (bitset.get(hash) && elements[hash].first != key) {
            hash = (hash + 1) % capacity;
        }
        return hash;
    }
public:
    const Value& operator[] (const Key& key) const {
        return elements[locate(key)];
    }
    Value& operator[] (const Key& key) {
        auto index = locate(key);
        if (!bitset.get(index)) {
            ++count;
            bitset.set(index);
            elements[index] = { key, {} };
            if (count > capacityThreshold) {
                resize(capacity * 2);
            }
        }
        return elements[index];
    }
    void insert (const KeyValue& kv) {
        auto index = locate(key);
        elements[index] = kv;
        if (!bitset.get(index)) {
            ++count;
            bitset.set(index);
            if (count > capacityThreshold) {
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
        for (; begin != end; ++begin) {
            insert(*begin);
        }
    }
private:
    template <typename T>
    class Iterator {
        Hashtable& hashtable;
        size_t     index = 0;

        Iterator (Hashtable& hashtable, size_t index = 0)
            : hashtable(hashtable), index(index) {}
    public:
        Iterator (const Iterator<T>& other) : hashtable(other.hashtable), index(index) {}
        Iterator& operator= (const Iterator<T>& other) { hashtable = other.hashtable; index = other.index; return *this; }
        operator bool () const { return index < hashtable.capacity; }

        template <typename T2> bool operator== (const Iterator<T2>& other) const { return hashtable == other.hashtable && index == other.index; } 
        template <typename T2> bool operator!= (const Iterator<T2>& other) const { return hashtable != other.hashtable || index != other.index; }

        Iterator<T>& operator++ () {
            if (i < hashtable.count) {
                for (++i; i < hashtable.count && !hashtable.bitset.set(i); ++i);
            }
            return *this; 
        }
        T& operator*  () const { return hashtable.elements[i]; }
        T* operator-> () const { return hashtable.elements[i]; }
    };
public:
    typedef Iterator<KeyValue> iterator;
    typedef Iterator<const KeyValue> const_iterator;

    iterator begin () { return iterator(*this, 0); }
    iterator end   () { return iterator(*this, capacity); }
    const_iterator begin () const { return const_iterator(*this, 0); }
    const_iterator end   () const { return const_iterator(*this, capacity); }

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

#endif // HashTable_h
