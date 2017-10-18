// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Simulation.cpp
//
// Implements a simpler server simulation using our Queue implementation.
// Note: not a real server, not threadsafe, does not do anything or run in
// real time, etc.
//
// remote source: https://github.com/SeijiEmery/comp220/blob/master/assignment_07/src/Simulation.cpp
//

#include <iostream>
#include <string>
using namespace std;

#include <cstdlib>
#include <cmath>
#include "Queue.h"

int main () {
    std::cout << "Programmer:       Seiji Emery\n";
    std::cout << "Programmer's ID:  M00202623\n";
    std::cout << "File:             " << __FILE__ << '\n' << std::endl;

    return 0;
}


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
