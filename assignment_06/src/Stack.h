// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Stack.h
//
// Remote source:
// https://github.com/SeijiEmery/comp220/tree/master/assignment_06/src/Stack.h
//

#ifndef Stack_h
#define Stack_h
#include <cassert>

// Simple reference stack implementation based on a linked list.
template <typename T>
class Stack {
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
    Stack () {}
    Stack (const T& value) { push(value); }
    Stack (const Stack<T>& other) : 
        head(other.head ? new Node(*(other.head)) : nullptr), 
        length(other.length) 
    {}
    ~Stack () { clear(); }

    Stack<T>& operator= (const Stack<T>& other) {
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

#endif // Stack_h
