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
class LinkedListStack {
    struct Node {
        T     value;
        Node* prev = nullptr;

        Node () {}
        Node (const T& value, Node* prev = nullptr) : 
            value(value), 
            prev(prev) 
        {}
        Node (const Node& node) : 
            value(node.value), 
            prev(node.prev ? new Node(*(node.prev)) : nullptr) 
        {}
        ~Node () { if (prev) delete prev; }
    };
    Node* head = nullptr;
    size_t length = 0;
public:
    LinkedListStack () {}
    LinkedListStack (const T& value) { push(value); }
    LinkedListStack (const LinkedListStack<T>& other) : 
        head(other.head ? new Node(*(other.head)) : nullptr), 
        length(other.length) 
    {}
    ~LinkedListStack () { clear(); }

    LinkedListStack<T>& operator= (const LinkedListStack<T>& other) {
        clear();
        if (other.head) {
            head = new Node(*(other.head));
        }
        length = other.head;
        return *this;
    }
    void push (const T& value) {
        head = new Node(value, head);
        ++length;
    }
    T& peek () { 
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
};

#endif // Stack_h
