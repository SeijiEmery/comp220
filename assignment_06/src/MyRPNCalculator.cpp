// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// MyRPNCalculator.cpp
//
// Implements a RPN calculator / interpreter. Supported operations:
//  'a b +' => a + b    'a +' => does nothing
//  'a b -' => a - b    'a -' => -a
//  'a b *' => a * b    'a *' => does nothing
//  'a b /' => a / b    'a /' => 1 / a
//  'a b ^' => pow(a,b) 'a ^' => does nothing
//
// Plus math functions:
//  sqrt, abs, log
//  sin, cos, tan, asin, acos, atan
//
//  'disp'  => prints entire stack (aliased '.')
//  'top'   => prints top of stack
//  'swap'  => swaps 2 top elements on stack
//  'dup'   => duplicates top element on stack
//  'drop'  => removes top of stack
//  'clear' => clears entire stack
//
//  'help'  => print a (not very useful) help message
//  'mem'   => prints memory usage + resets memory usage counters
//       Note: fairly limited, as not originally written for this
//
//  'q' / 'Q' => exits the program (or use ctrl+C)
//  

#include <iostream>
#include <string>
#include <regex>
#include <memory>
#include <functional>
using namespace std;

#include <cstdlib>
#include <cmath>
#include "Stack.h"

// #define ASSIGNMENT_SPEC     // for a more interesting program, comment out this line

//
// Memory + time benchmarking code: this hijacks (overloads) global new / delete
// to trace memory allocations (very simple: # of allocations / frees + # bytes
// allocated / freed), and adds a global variable that displays this stuff
// from its dtor (guaranteed to be called after main() but before program exits).
//
// It also adds basic time profiling (global ctor / dtor) using std::chrono.
//
// All of this can be achieved externally ofc using time + valgrind (*nix),
// and is perhaps preferable - but implementing these interally was an interesting
// exercise nevertheless.
//
// This can all be disabled if compiling with -D NO_MEM_DEBUG.
//
#ifndef NO_MEM_DEBUG
#include <chrono>

struct MemTracer {
    void traceAlloc (size_t bytes) { ++numAllocations; allocatedMem += bytes; }
    void traceFreed (size_t bytes) { ++numFrees; freedMem += bytes;}
private:
    size_t numAllocations = 0;  // number of allocations in this program
    size_t numFrees       = 0;  // number of deallocations in this program
    size_t allocatedMem   = 0;  // bytes allocated
    size_t freedMem       = 0;  // bytes freed

    std::chrono::high_resolution_clock::time_point t0;  // time at program start
public: 
    void memstat () {
        std::cout << "\nUsed  memory: " << ((double)allocatedMem) * 1e-6 << " MB (" << numAllocations << " allocations)\n";
        std::cout << "Freed memory: "   << ((double)freedMem)     * 1e-6 << " MB (" << numFrees       << " deallocations)\n";
        numAllocations = 0;
        numFrees = 0;
        allocatedMem = 0;
        freedMem = 0;
    }

    MemTracer () : t0(std::chrono::high_resolution_clock::now()) {}
    ~MemTracer () {
        using namespace std::chrono;
        auto t1 = high_resolution_clock::now();
        std::cout << "\nUsed  memory: " << ((double)allocatedMem) * 1e-6 << " MB (" << numAllocations << " allocations)\n";
        std::cout << "Freed memory: "   << ((double)freedMem)     * 1e-6 << " MB (" << numFrees       << " deallocations)\n";
        std::cout << "Ran in " << duration_cast<duration<double>>(t1 - t0).count() * 1e3 << " ms\n";
    }
} g_memTracer;

void* operator new (size_t size) throw(std::bad_alloc) {
    g_memTracer.traceAlloc(size);
    size_t* mem = (size_t*)std::malloc(size + sizeof(size_t));
    if (!mem) {
        throw std::bad_alloc();
    }
    mem[0] = size;
    return (void*)(&mem[1]);
}
void operator delete (void* mem) throw() {
    auto ptr = &((size_t*)mem)[-1];
    g_memTracer.traceFreed(ptr[0]);
    std::free(ptr);
}

#endif // NO_MEM_DEBUG

template <typename T>
T popBack (Stack<T>& stack, T default_ = T()) {
    T back = stack.empty() ? default_ : stack.peek();
    stack.pop();
    return back;
}

struct InterpreterState;
struct Type;

struct Value {
    virtual ~Value () {}
    virtual const Type& type () const = 0;
    virtual void repr (std::ostream&) const = 0;
    virtual int  cmp  (const Value&)  const = 0;
};
typedef std::unique_ptr<Value> ValuePtr;

template <typename T, typename... Args>
ValuePtr makeValue (Args... args) { return ValuePtr(static_cast<Value*>(new T(args...))); }

template <typename T>
T& as (ValuePtr& value) {
    return *dynamic_cast<T*>(value.get());
}
template <typename T>
T& as (Value& value) {
    return *dynamic_cast<T*>(&value);
}
template <typename T>
const T& as (const Value& value) {
    return *dynamic_cast<const T*>(&value);
}

class Type : Value {
    const std::string name;
    int               id;
    static int nextId;
public:
    Type (const std::string& name) : name(name), id(nextId++) {}
    const Type& type () const override;
    void  repr (std::ostream& os) const override { os << name; }
    
    int cmp (const Type& other) const {
        return id - other.id;
    }
    int cmp (const Value& other) const override {
        return type().cmp(other.type()) || 
            (id - as<Type>(other).id);
    }
    friend bool operator== (const Type& a, const Type& b) { return a.id == b.id; }
    friend bool operator!= (const Type& a, const Type& b) { return a.id != b.id; }
};
int Type::nextId = 0;


// Global type instances for builtin types.
// This forms the foundation of our dynamic type system (similar to python):
//  type(10) => number, type(number) => type, type(type) => type
//
Type gt_nil     { "nil" };
Type gt_type    { "type"   };
Type gt_number  { "number" };
Type gt_string  { "string" };
Type gt_bool    { "bool"   };
Type gt_block   { "block"  };
Type gt_builtin { "builtin" };
Type gt_list    { "list" };
Type gt_dict    { "dict" };

const Type& Type::type () const { return gt_type; }

struct Nil : Value {
    const Type& type () const override { return gt_nil;  }
    void repr (std::ostream& os) const override { os << "nil"; }
    int cmp (const Value& other) const override {
        return type().cmp(other.type());
    }
} nil;
struct Number : Value {
    double value;

    Number (double value) : value(value) {}
    const Type& type () const override { return gt_number; }
    void repr (std::ostream& os) const override { os << value; }
    int cmp (const Value& other) const override {
        return type().cmp(other.type()) ||
            (value > as<Number>(other).value ? 1 : value < as<Number>(other).value ? -1 : 0);
    }
};
struct Bool : Value {
    bool value;

    Bool (bool value) : value(value) {}
    const Type& type () const override { return gt_bool; }
    void repr (std::ostream& os) const override { os << (value ? "true" : "false"); }
    int cmp (const Value& other) const override {
        return type().cmp(other.type()) ||
            (value == as<Bool>(other).value ? 0 : -1);
    }
};
struct String : Value {
    std::string value;

    String (std::string value) : value(value) {}
    const Type& type () const override { return gt_string; }
    void repr (std::ostream& os) const override { os << '"' << value << '"'; }
    int cmp (const Value& other) const override {
        return type().cmp(other.type()) ||
            value.compare(as<String>(other).value);
    }
};
struct Block : Value {
    std::string code;

    Block (std::string code) : code(code) {}
    const Type& type () const override { return gt_block; } 
    void repr (std::ostream& os) const override { os << '{' << code << '}'; }
    int cmp (const Value& other) const override {
        return type().cmp(other.type()) ||
            code.compare(as<Block>(other).code);
    }
};
struct Builtin : Value {
    std::string name;
    void (*builtin)(InterpreterState&);

    Builtin (std::string name, void(*builtin)(InterpreterState&)) : name(name), builtin(builtin) {}
    const Type& type () const override { return gt_builtin; }
    void repr (std::ostream& os) const override { os << '{' << name << '}'; }
    int cmp (const Value& other) const override {
        return type().cmp(other.type()) ||
            (builtin == as<Builtin>(other).builtin ? 0 : -1);
    }
};
struct List : Value {
    const Type& type () const override { return gt_list; }
    void repr (std::ostream& os) const override {}
    int cmp (const Value& other) const override {
        int rv = -1; return type().cmp(other.type()) || rv;
    }
};
struct Dict : Value {
    const Type& type () const override { return gt_dict; }
    void repr (std::ostream& os)  const override {}
    int cmp (const Value & other) const override {
        int rv = -1; return type().cmp(other.type()) || rv;
    }
    void insert (const std::string& key, ValuePtr&& value) {}
    int find (const std::string key) { return -1; }
    int end  () { return 0; }
};

typedef std::unique_ptr<Dict>  DictPtr;



struct InterpreterState {
    Stack<ValuePtr> stack;
    Stack<DictPtr>  dict_stack;

    // Stack operations

    template <typename T, typename... Args>
    void push (Args... args) { stack.push(makeValue<T>(args...)); }

    ValuePtr pop () {
        if (stack.empty()) {
            return std::unique_ptr<Value>(static_cast<Value*>(new Nil()));
        } else {
            auto value = std::move(stack.peek());
            stack.pop();
            return value;
        }
    }

    // void pushScope () { dict_stack.push(std::move(std::unique_ptr<Dict>(new Dict()))); }
    void popScope  () { dict_stack.pop(); }
    void define    () {
        if (stack.size() >= 2) {
            auto lhs = pop();
            auto rhs = pop();
            if      (lhs->type() == gt_string) { dict_stack.peek()->insert(as<String>(lhs).value, std::move(rhs)); }
            else if (rhs->type() == gt_string) { dict_stack.peek()->insert(as<String>(rhs).value, std::move(lhs)); }
            else {
                // stack.push(std::move(rhs));
                // stack.push(std::move(lhs));
            }
        }
    }
    void define (std::string name, void (*fcn)(InterpreterState&)) {
        dict_stack.peek()->insert(name, makeValue<Builtin>(name, fcn));
    }
    // Value& lookup (std::string name) {
    //     auto it = dict_stack.peek()->find(name);
    //     if (it != dict_stack.peek()->end()) {
    //         return it->get();
    //     } else {
    //         return static_cast<Value&)(nil);
    //     }
    // }

    void interpret (const std::string& code) {
        // ...
    }

    template <typename A, typename B>
    void exec (std::function<void(A&,B&)> fcn, bool allowSwappedOperands = false) {
        if (stack.size() >= 2) {
            auto lhs = pop();
            auto rhs = pop();
            if (lhs->type() == A::type() && rhs->type() == B::type()) { fcn(*as<A>(lhs), *as<B>(rhs)); return; }
            else if (!allowSwappedOperands) {}
            else if (rhs->type() == A::type() && lhs->type() == B::type()) { fcn(*as<A>(rhs), *as<B>(lhs)); return; }
        }
    }
    template <typename... Args>
    std::function<void(InterpreterState&)> delayedExec (Args... args) {
        return [args...](InterpreterState& interpreter){
            interpreter.exec(args...);
        };
    }

    void setupBuiltins () {
        define("+",    delayedExec([this](Number& a, Number& b){ push<Number>(a.value + b.value); }));
        define("-",    delayedExec([this](Number& a, Number& b){ push<Number>(a.value - b.value); }));
        define("*",    delayedExec([this](Number& a, Number& b){ push<Number>(a.value * b.value); }));
        define("/",    delayedExec([this](Number& a, Number& b){ push<Number>(a.value / b.value); }));
        define("^",    delayedExec([this](Number& a, Number& b){ push<Number>(pow(a.value, b.value)); }));
        define("sqrt", delayedExec([this](Number& a){ push<Number>(sqrt(a)); }));
        define("log",  delayedExec([this](Number& a){ push<Number>(log(a)); }));

        define("define", [](InterpreterState& interpreter){ interpreter.define(); });
        define("dup",    [](InterpreterState& interpreter){ interpreter.stack.push(interpreter.stack.peek()); });
    }
    {    
        int x = 10;
        std::function<T(T,T)> plus = [x](T a, T b) -> T { return x + a + b; }

        plus(a, b);
        plus = Functor(x);
        plus(a, b);
    }

    T(*fcn_ptr)(T,T); 
};

struct Functor : Foo {
    int x;
    Functor (int x) : x(x) {}
    T operator ()(T a, T b) { return a + b; }
};
{
    Functor foo;
    Foo* f = static_cast<Foo*>(&foo);
}

struct Foo {
    // virtual ~Foo () {}
    virtual dtor (Foo&) {}
};

struct Foo {
    Foo_vtbl* vtbl;
    // members...

    struct Foo_vtbl {
        void (*dtor)(Foo&);
    };
};

struct IAnimal {
    virtual void speak () = 0;
};
struct Dog : IAnimal {
    void speak () override { std::cout << "woof!" << std::endl; }
};
struct Cat : IAnimal {
    void speak () override { std::cout << "meow!" << std::endl; }
};

void example () {
    std::vector<IAnimal*> animals;
    animals.push_back((IAnimal*)new Dog());
    animals.push_back(static_cast<IAnimal*>(new Cat()));

    for (auto animal : animals) {
        animal->speak();
    }
}


int main () {
    std::cout << "Programmer:       Seiji Emery\n";
    std::cout << "Programmer's ID:  M00202623\n";
    std::cout << "File:             " << __FILE__ << '\n' << std::endl;

    Stack<double> values;
    std::string line, input;
    bool        running = true;

    std::regex  expr { "\\s*(\\d+((\\.|[eE]\\-?)\\d+)*|[\\+\\-\\*\\^\\.\\/qQ]|help|drop|dup|disp|swap|clear|top|pi|exp|e|sin|cos|tan|asin|acos|atan|sqrt|log|abs|mem|hex|bin)\\s*" };
    std::smatch match;
    double a, b;

    auto writeStack = [&values](){
        if (values.empty()) {
            #ifndef ASSIGNMENT_SPEC
                std::cout << "empty ";
            #endif
        } else {
            auto copy = values;
            while (!copy.empty()) {
                std::cout << copy.peek() << " ";
                copy.pop();
            }
        }
    };
    auto displayStack = [&writeStack](){
        writeStack(); std::cout << "\n";
    };

    std::cout << "\033[36;1m";
    while (running) {
        // #ifdef ASSIGNMENT_SPEC
            writeStack();
        // #endif
        std::cout << "\033[0m";
        while (!getline(cin, line));
        // std::cin >> line;
        std::cout << "\033[36;1m";
        int opcount = 0;    // # operations since last disp / eol
        for (; std::regex_search(line, match, expr); line = match.suffix().str()) {
            auto token = match[1].str();
            // std::cout << "Token '" << token << "'\n";
            #define CASE_BIN_OP(chr,op) \
                case chr: values.push(popBack(values) op popBack(values)); break;

            switch (token[0]) {
                case '+': { auto rhs = popBack(values), lhs = popBack(values, 0.0); values.push(lhs + rhs); } break;
                case '-': { auto rhs = popBack(values), lhs = popBack(values, 0.0); values.push(lhs - rhs); } break;
                case '*': { auto rhs = popBack(values), lhs = popBack(values, 1.0); values.push(lhs * rhs); } break;
                case '/': { auto rhs = popBack(values), lhs = popBack(values, 1.0); values.push(lhs / rhs); } break;
                case '^': { auto rhs = popBack(values), lhs = popBack(values, 1.0); values.push(pow(lhs, rhs)); } break;
                case 'd': 
                    if (token == "drop") { values.pop(); } 
                    else if (token == "dup" && !values.empty()) { values.push(values.peek()); }
                    else if (token == "disp") { displayStack(); }
                    break;
                case 't': 
                    if (token == "top") {
                        if (values.empty()) std::cout << "empty\n";
                        else std::cout << values.peek() << '\n';
                    } else if (token == "tan") {
                        values.push(tan(popBack(values)));
                    } 
                    break;
                case 's': 
                    if (token == "swap" && values.size() >= 2) { 
                        // values.swap();
                        auto top = popBack(values), btm = popBack(values); 
                        values.push(top); values.push(btm);
                    } else if (token == "sin" && values.size() >= 1) {
                        values.push(sin(popBack(values)));
                    } else if (token == "sqrt") {
                        values.push(sqrt(popBack(values)));
                    }
                break;
                case 'e':
                    if (token == "e") {
                        values.push(exp(1.0));
                    } else if (token == "exp") {
                        values.push(exp(popBack(values)));
                    }
                break;
                case 'c': 
                    if (token == "clear") {
                        values.clear();
                    } else if (token == "cos" && values.size() >= 1) {
                        values.push(cos(popBack(values)));    
                    }
                break;
                case 'a':
                    if (token == "asin") {
                        values.push(asin(popBack(values)));
                    } else if (token == "acos") {
                        values.push(acos(popBack(values)));
                    } else if (token == "atan") {
                        values.push(atan(popBack(values)));
                    } else if (token == "abs") {
                        values.push(fabs(popBack(values)));
                    }
                break;
                case 'p':
                    if (token == "pi") {
                        // clever way to calculate pi – from https://stackoverflow.com/a/1727886
                        // values.push(atan(1) * 4);
                        values.push(3.141592653589793);
                    }
                break;
                case 'm': 
                    if (token == "mem") {
                        g_memTracer.memstat();
                    }
                break;
                case 'h': 
                    if (token == "help") {
                        std::cout << "RPN Calculator. Enter number, or any of the following operations:\n"
                            << "\t+, -, *, /, ^, sin, cos, tan, acos, asin, atan, sqrt, abs, drop, dup, disp (alias .), swap, quit, clear, top, help\n";
                    } else if (token == "hex" && !values.empty()) {
                        std::cout << "0x" << std::hex << (int)values.peek() << '\n';
                    } 
                break;
                case 'l':
                    if (token == "log") {
                        values.push(log(popBack(values)));
                    }
                    break;
                case 'b':
                    if (token == "bin" && !values.empty()) {
                        uint64_t i = *reinterpret_cast<uint64_t*>(&values.peek()) << '\n';
                        char bit[] = { '0', '1' };
                        std::cout << "0b";
                        if (!i) std::cout << "0"; 
                        while (i) {
                            std::cout << bit[i & 1];
                            i >>= 1; 
                        }
                        std::cout << '\n';
                    }
                break;
                case 'q':
                    if (token == "quit") { running = false; goto quit; }
                    break;
                // case 'q': case 'Q': running = false; goto quit;
                case '.': displayStack(); break;
                default: number: values.push(atof(token.c_str()));
            }
            ++opcount;
        }
    quit:
        continue;
    }
    return 0;
}
