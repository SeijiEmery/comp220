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

int main () {
    std::cout << "Programmer:       Seiji Emery\n";
    std::cout << "Programmer's ID:  M00202623\n";
    std::cout << "File:             " << __FILE__ << '\n' << std::endl;

    Stack<double> values;
    std::string line, input;
    bool        running = true;

    std::regex  expr { "\\s*([0-9\\.eE]+|[\\+\\-\\*\\^\\/qQ]|help|drop|dup|disp|swap|clear|top|pi|sin|cos|tan|asin|acos|atan|sqrt|abs|mem|hex|bin)\\s*" };
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
        #ifdef ASSIGNMENT_SPEC
            writeStack();
        #endif
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
                        auto top = popBack(values), btm = popBack(values); 
                        values.push(top); values.push(btm);
                    } else if (token == "sin" && values.size() >= 1) {
                        values.push(sin(popBack(values)));
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
                case 'q': case 'Q': running = false; goto quit;
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
