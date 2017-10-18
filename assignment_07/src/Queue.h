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
};

#endif // Queue_h
