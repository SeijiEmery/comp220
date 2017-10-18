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
        Node (T&& value, Node* prev = nullptr) :
            value(value),
            prev(prev)
        {}
        Node (const T& value, Node* prev = nullptr) : 
            value(value), 
            prev(prev) 
        {}
        Node (const Node& node) : 
            value(node.value), 
            prev(node.prev ? new Node(*(node.prev)) : nullptr) 
        {}
        ~Node () {}
    };
    Node* head = nullptr;
    size_t length = 0;
public:
    Queue () {}
    Queue (const T& value) { push(value); }
    Queue (const Queue<T>& other) : 
        head(other.head ? new Node(*(other.head)) : nullptr), 
        length(other.length) 
    {}
    ~Queue () { clear(); }

    Queue<T>& operator= (const Queue<T>& other) {
        clear();
        if (other.head) {
            head = new Node(*(other.head));
        }
        length = other.head;
        return *this;
    }
    void push (T&& value) {
        head = new Node(value, head);
        ++length;
    }
    void push (const T& value) {
        head = new Node(value, head);
        ++length;
    }
    T& peek () { 
        assert(!empty()); 
        return head->value; 
    }
    const T& peek () const {
        assert(!empty());
        return head->value;
    }
    void pop () {
        if (head) {
            Node* prev = head->prev;
            delete head;
            head = prev;
            --length;
        }
    }
    size_t size () const { 
        return length; 
    }
    bool empty () const {
        assert((length == 0) == (head == nullptr));
        return head == nullptr;
    }
    void clear () {
        while (!empty()) {
            pop();
        }
    }
    void swap () {
        if (size() >= 2) {
            Node* prev = head;
            head = head->prev;
            prev->prev = head->prev;
            head->prev = prev;
        }
    }
};

#endif // Queue_h
