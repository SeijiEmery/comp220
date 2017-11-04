
#ifndef AssociativeArray_h
#define AssociativeArray_h

template <typename Key, typename Value, typename Impl<class,class>>
class AssociativeArray : public Impl<Key, Value> {
public:
    typedef Key Key;
    typedef Value Value;

    AssociativeArray () : Impl<Key, Value> {}
    AssociativeArray (const AssociativeArray<Key, Value>& other) { 
        this->insert(other.begin(), other.end()); 
    }
    AssociativeArray (const AssociativeArray<Key, Value>& other) {
        this->swap(other);
    }
    AssociativeArray (const std::initializer_list<std::pair<Key, Value>>& values) {
        this-insert(values.begin(), values.end());
    }
    AssociativeArray<Key, Value>& operator= (const AssociativeArray<Key, Value>& other) {
        if (this != &other) {
            this->clear();
            this->insert(other.begin(), other.end());
        }
        return *this;
    }
    AssociativeArray<Key, Value>& operator= (AssociativeArray<Key, Value>&& other) {
        if (this != other) {
            this->swap(other);
        }
        return *this;
    }
    ~AssociativeArray () {
        this->clear();
    }
    bool containsKey (const Key& key) const { return this->contains(key); }
    void deleteKey   (const Key& key) const { this->remove(key); }

    template <typename It, typename V>
    class Iterator {
        It it;

        template <typename... Args>
        Iterator (Args... args) : it(args...) {}
    public:
        Iterator () : it() {}
        Iterator (const Iterator&) = default;
        Iterator& operator= (const Iterator&) = default;
        ~Iterator () {}

        friend bool operator== (const Iterator<V>& a, const Iterator<V>& b) {
            return cmp(a.it, b.it) == 0;
        }
        friend bool operator!= (const Iterator<V>& a, const Iterator<V>& b) {
            return cmp(a.it, b.it) != 0;
        }
        operator bool () const { return bool(it); }
        const V& operator*  () const { return static_cast<const V&>(*(it.get())); }
        V&       operator*  ()       { return static_cast<V&>(*(it.get())); }
        const V* operator-> () const { return static_cast<const V*>(it.get()); }
        V*       operator-> ()       { return static_cast<V*>(it.get()); }
        Iterator<V>& operator++ ()   { return it.advance(), *this; }
        operator Iterator<const V> () const { return { it }; }
    };

    typedef Iterator<Impl<Key,Value>::KeyIterator, Key>         iterator;
    typedef Iterator<Impl<Key,Value>::KeyIterator, const Key>   const_iterator;

    iterator begin () { return iterator(static_cast<Impl<Key,Value>*>(this)); }
    iterator end   () { return iterator(); }

    const_iterator begin () { return const_iterator(static_cast<Key,Value>*>(this)); }
    const_iterator end   () { return const_iterator(); }

    const_iterator cbegin () const { return begin(); }
    const_iterator cend   () const { return end();   }
};

template <typename K, typename V>
class ListImpl {
    struct Node {
        std::pair<K,V> kv;
        Node* next = nullptr;
        Node (const K& key, const V& value) : kv(key, value) {}
        friend int cmp (const Node* a, const Node* b) {
            return static_cast<ptrdiff_t>(b) - static_cast<ptrdiff_t>(a);
        }
    };
    Node* head = nullptr;
protected:
    struct BaseIterator {
        Node* node;
        BaseIterator (const Node* node = nullptr) : node(node) {}
        BaseIterator (const BaseIterator&) = default;
        BaseIterator& operator= (const BaseIterator&) = default;
        ~BaseIterator () {}

        operator bool () const { return node != nullptr; }
        void advance () { if (node) node = node->next; }
    };
    struct KeyIterator : public BaseIterator {
        KeyIterator (ListImpl<K,V>& self) : BaseIterator(self.node) {}
        K& get () { return node->kv.first; }
        const K& get () const { return node->kv.first; }
        friend int cmp (const KeyIterator& a, const KeyIterator& b) { return cmp(a.node, b.node); }
    };
    struct ValueIterator : public BaseIterator {
        ValueIterator (ListImpl<K,V>& self) : BaseIterator(self.node) {}
        V& get () { return node->kv.second; }
        const V& get () const { return node->kv.second; }
        friend int cmp (const KeyIterator& a, const KeyIterator& b) { return cmp(a.node, b.node); }
    };
    struct Iterator : public BaseIterator {
        Iterator (ListImpl<K,V>& self) : BaseIterator(self.node) {}
        std::pair<K,V>& get () { return node->kv; }
        const std::pair<K,V>& get () const { return node->kv; }
        friend int cmp (const KeyIterator& a, const KeyIterator& b) { return cmp(a.node, b.node); }
    };
public:
    ListImpl () {}
    ListImpl (const ListImpl&) {}
    ListImpl& operator= (const ListImpl&) {}

    V& operator[] (const K& key) {
        for (Node*& node = head; true; node = node->next) {
            if (node == nullptr) {
                return (node = new Node(key, {}, nullptr))->value;
            } else if (node.key == key) {
                return node.value;
            }
        }
    }
    const V& operator[] (const K& key) const {
        for (Node* node = head; node != nullptr; node = node->next) {
            if (node->key == key) {
                return node->value;
            }
        }
        return {};
    }
    bool contains (const K& key) const {
        for (Node* node = head; node != nullptr; node = node->next) {
            if (node->key == key) {
                return true;
            }
        }
        return false;
    }
    void insert (const K& key, const V& value) {
        (*this)[key] = value;
    }
    void insert (const std::pair<K,V> kv) {
        insert(kv.first, kv.second);
    }
    template <typename It>
    void insert (It begin, It end) {
        for (; begin != end; ++begin) {
            insert(*begin);
        }
    }
};

template <typename K, typename V>
struct ArrayImpl {

};

template <typename K, typename V>
struct BTreeImpl {

};

#endif // AssociativeArray_h
