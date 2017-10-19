// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Queue.h
//
// Remote source:
// https://github.com/SeijiEmery/comp220/tree/master/assignment_07/src/Queue.h
//

#ifndef Queue_h
#define Queue_h
#include <cassert>
#include <iterator>     // std::iterator
#include <type_traits>  // std::remove_cv


// Simple reference stack implementation based on a linked list.
template <typename T>
class Queue {
    struct Node {
        T     value;
        Node* prev = nullptr;

        Node () {}
        Node (T&& value) : value(value) {}
        Node (const T& value) : value(value) {}
        ~Node () {}
    };
    Node* head = nullptr;
    Node* tail = nullptr;
    size_t length = 0;

public:
    Queue () {}
    Queue (const T& value) { push(value); }
    Queue (const Queue<T>& other) { *this = other; }
    ~Queue () { clear(); }

    Queue<T>& operator= (const Queue<T>& other) {
        clear();
        for (Node* node = other.head; node; node = node->prev) {
            push(new Node(node->value));
        }
        assert(size() == other.size());
        return *this;
    }
protected:
    void push (Node* node) {
        assert(node != nullptr);
        if (!tail) { head = node; }
        else { tail->prev = node; }
        tail = node;
        ++length;
    }
public:
    void push (T&& value)      { push(new Node(value)); }
    void push (const T& value) { push(new Node(value)); }

    void pop () {
        assert(!empty());
        auto prev = head->prev;
        delete head;
        head = head->prev;
        if (!head) { tail = head; }
        --length;
    }

    T& front () { assert(!empty()); return head->value; }
    T& back  () { assert(!empty()); return tail->value; }

    const T& front () const { assert(!empty()); return head->value; }
    const T& back  () const { assert(!empty()); return tail->value; }

    bool empty () const {
        assert((head == nullptr) == (tail == nullptr));
        return head == nullptr;
    }
    size_t size () const { 
        assert(empty() == (length == 0)); 
        return length; 
    }
    void clear () {
        while (!empty()) {
            pop();
        }
        assert(empty());
        assert(length == 0);
    }

private:
    template <typename V, typename UT = std::remove_cv<V>>
    class Iterator : std::iterator<std::forward_iterator_tag, UT, std::ptrdiff_t, V*, V&> {
        Node* node = nullptr;
        Iterator (Node* node) : node(node) {}
        friend class Queue<T>;
    public:
        Iterator () {}
        operator bool () const { return node != nullptr; }
        void swap (Iterator& other) noexcept { std::swap(node, other.node); }
        Iterator& operator++ ()   { assert(node); node = node->prev;    return *this; }
        Iterator operator++ (int) { assert(node); Iterator copy(*this); return (*this)++, copy; }
        template <typename U> bool operator== (const Iterator<U>& other) const { return node == other.node; }
        template <typename U> bool operator!= (const Iterator<U>& other) const { return node != other.node; }
        V& operator* () const { assert(node); return node->value; }
        V& operator-> () const { assert(node); return node->value; }
        operator Iterator<const V> () const { return Iterator<const V>(node); }
    };
public:
    typedef Iterator<T>         iterator;
    typedef Iterator<const T>   const_iterator;

    iterator begin () { return iterator(head); }
    iterator end   () { return iterator(nullptr); }

    const_iterator begin () const { return const_iterator(head); }
    const_iterator end   () const { return const_iterator(nullptr); }

    const_iterator cbegin () const { return begin(); }
    const_iterator cend   () const { return end(); }
};

#endif // Queue_h
