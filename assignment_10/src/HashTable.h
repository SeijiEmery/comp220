#ifndef HashTable_h
#define HashTable_h

template <typename Key, typename Value, size_t CAPACITY>
class HashTable {
public:
    typedef HashTable<Key,Value,CAPACITY>   This;
    typedef std::pair<Key, Value>           KeyValue;
    typedef size_t(*HashFunction)(const Key&);
private:
    HashFunction hashFunction;
    KeyValue*    data;
public:
    HashTable (HashFunction = nullptr) 
        : hashFunction(hashFunction),
          data(new KeyValue[CAPACITY])
    {
        std::fill(&data[0], &data[CAPACITY], {});
    }
    ~HashTable () { delete data; }

    HashTable (const This& other) : HashTable() { *this = other; }
    HashTable (This&& other) : hashFunction(nullptr), data(nullptr) { *this = std::move(other); }
    This& operator= (const This& other) { 
        hashFunction = other.hashFunction; 
        std::copy(&other.data[0], &other.data[CAPACITY], &data[0]);
        count = other.count;
        return *this;
    }
    This& operator= (This&& other) {
        std::swap(hashFunction, other.hashFunction);
        std::swap(data, other.data);
        std::swap(count, other.count);
        return *this;
    }
private:
    size_t locate (const Key& key) {
        assert(hashFunction != nullptr);
        assert(data != nullptr);
        size_t index = hashFunction(key) % CAPACITY;

        for (size_t i = CAPACITY; i --> 0; ) {
            if (data[index].key == key) {
                return index;
            } else if (data[index].key == Key()) {
                return index;
            } else {
                index = (index + 1) % CAPACITY;
            }   
        }
        assert(0 && "Hashtable capacity exceeded!");
    }
public:
    Value& operator[] (const Key& key) { 
        auto index = locate(key);
        if (data[index].key != key) {
            data[index].key = key;
            ++count;
        }
        return data[locate(key)]; 
    }
    const Value& operator[] (const Key& key) const {
        return data[locate(key)];
    }
    bool hasKey (const Key& key) const {
        return locate(key).key != Key();
    }
    void deleteKey (const Key& key) {
        // ...
    }
};

#endif // HashTable_h
