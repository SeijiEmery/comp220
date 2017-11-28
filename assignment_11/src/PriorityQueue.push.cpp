// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// PriorityQueue.push.cpp
//
// PriorityQueue timing test (push operations)
//

#include <iostream>
#include <vector>
using namespace std;

#include <ctime>
#include <cstdlib>
#include "PriorityQueue.h"

class BenchResult {
    clock_t startTime, endTime;
    bool started = false, stopped = false;
public:
    BenchResult () : startTime(0), endTime(0) {}

    // Start / stop bench time
    void start () { startTime = clock(); started = true; stopped = false; }
    void stop  () { endTime   = clock(); stopped = false; }

    // Returns true iff in a valid timing state (start() called, then stop()).
    bool valid () const { return started && stopped; }

    // Returns total time elapsed, in seconds
    double elapsed () const { return (double)(endTime - startTime) / CLOCKS_PER_SEC; }

    template <typename F>
    BenchResult& run (const F& inner) {
        start();
        inner();
        stop();
        return *this;
    }
};

template <typename F>
double benchmark (const F& inner) {
    return BenchResult().run(inner).elapsed();
}
template <typename F>
double benchmark (size_t iterations, const F& inner) {
    auto duration = benchmark([&](){
        for (size_t i = iterations; i --> 0; ) {
            inner();
        }
    });
    std::cout << "runtime: " << duration << '\n';
    return duration / static_cast<double>(iterations);
}

template <typename T, typename This>
class PriorityQueueBenchmark {
    std::vector<PriorityQueue<T>> items;
    std::vector<T>                data;
public:
    // resize / re-fill input data
    template <typename Generator>
    This& generate (size_t count, const Generator& generator) {
        data.clear();
        data.reserve(count);
        for (size_t i = count; i --> 0; ) {
            data.push_back(generator());
        }
        return static_cast<This&>(*this);
    }
    template <typename Reporter>
    This& run (size_t iterations, const Reporter& report) {
        items.clear();
        for (size_t i = iterations; i --> 0; ) {
            items.emplace_back();
        }
        size_t i = 0;
        report(iterations, benchmark(iterations, [&](){
            static_cast<This&>(*this).run(i, items[i], data); 
            ++i;
        }));
        return static_cast<This&>(*this);
    }
    This& runSuite (std::initializer_list<std::pair<size_t, size_t>> counts) {
        for (const auto& pair : counts) {
            size_t count = pair.first, iterations = pair.second;
            srand(time(nullptr));
            generate(count, [](){ return static_cast<T>(rand() % 2048 - 1024); });
            run(iterations, [&](size_t iterations, double benchDuration /* seconds */) {
                std::cout << "Ran " << iterations << " iterations, total " 
                    << benchDuration 
                    << " each " << (benchDuration / count * 1e6) << " µs "
                    << " for " << count << " element(s)\n";
            });
        }
        return static_cast<This&>(*this);
    }
};

template <typename T>
struct PriorityQueuePushBenchmark : public PriorityQueueBenchmark<T, PriorityQueuePushBenchmark<T>> {
    T accumulator;
    void run (size_t i, PriorityQueue<T>& queue, const std::vector<T>& data) {
        // std::cout << data.size() << " elements\n";
        for (const auto& element : data) {
            queue.push(element);
        }
        accumulator += queue.size();
        // std::cout << queue << '\n';
    }
    void info () {
        std::cout << accumulator << '\n';
    }
};

int main () {
    PriorityQueuePushBenchmark<double>()
        .runSuite({
            { 1,  1000 }, 
            { 100, 1000 },
            { 1000, 100 },
            { 10000, 100 },
            { 100000, 10 },
            { 1000000, 10 },
            { 10000000, 1 },
            // { 100000000, 1 },
        })
        .info();
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

