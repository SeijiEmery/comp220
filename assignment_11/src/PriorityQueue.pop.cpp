// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// PriorityQueue.pop.cpp
//
// PriorityQueue timing test (pop operations)
//

#include <iostream>
#include <iomanip>
#include <vector>
using namespace std;

#include <ctime>
#include <cstdlib>
#define NO_PQUEUE_DEBUG
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
    // std::cout << "runtime: " << duration << '\n';
    return duration / static_cast<double>(iterations);
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
    size_t totalMemory () const { return allocatedMem; }
    size_t totalAllocations () const { return numAllocations;  }
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


struct LocalMemoryTracer {
    size_t initialMemory = 0;
    size_t initialAllocs = 0;
    size_t usedMemory = 0;  // # bytes of memory allocated
    size_t usedAllocs = 0;  // # allocations

    void enter () {
        initialMemory = g_memTracer.totalMemory();
        initialAllocs = g_memTracer.totalAllocations();
    }
    void exit () {
        usedMemory = g_memTracer.totalMemory() - initialMemory;
        usedAllocs = g_memTracer.totalAllocations() - initialAllocs;
    }
};

struct Bytes {
    size_t bytes;
    Bytes (size_t bytes) : bytes(bytes) {}
    friend std::ostream& operator<< (std::ostream& os, const Bytes& self) {
        if (self.bytes < (1UL << 10)) return os << self.bytes << " bytes";
        if (self.bytes < (1UL << 20)) return os << (self.bytes * 1e-3) << " KB";
        if (self.bytes < (1UL << 30)) return os << (self.bytes * 1e-6) << " MB";
        if (self.bytes < (1UL << 40)) return os << (self.bytes * 1e-9) << " GB";
        return os << (self.bytes * 1e-12) << " TB";
    }
};
struct Seconds {
    double seconds;
    Seconds (double seconds) : seconds(seconds) {}
    friend std::ostream& operator<< (std::ostream& os, const Seconds& self) {
        if (self.seconds > 1.0)  return os << self.seconds << " sec";
        if (self.seconds > 1e-3) return os << (self.seconds * 1e3) << " ms";
        if (self.seconds > 1e-6) return os << (self.seconds * 1e6) << " µs";
        return os << (self.seconds * 1e9) << " ns";
    }
};


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
        for (size_t i = 0; i < iterations; ++i) {
            items.emplace_back();
            static_cast<This&>(*this).before(i, items[i], data);
        }
        size_t i = 0;
        report(iterations, benchmark(iterations, [&](){
            static_cast<This&>(*this).run(i, items[i], data); 
            ++i;
        }));
        return static_cast<This&>(*this);
    }
    This& runSuite (std::initializer_list<std::pair<size_t, size_t>> counts) {
        LocalMemoryTracer memoryTracer;
        double expected = 5e-9;

        srand(time(nullptr));
        for (const auto& pair : counts) {
            size_t count = pair.first, iterations = pair.second;
            generate(count, [](){ return static_cast<T>(rand() % 2048 - 1024); });

            memoryTracer.enter();
            run(iterations, [&](size_t iterations, double benchDuration /* seconds */) {
                memoryTracer.exit();
                Seconds duration { benchDuration };
                Seconds projected { expected * count };
                Bytes usedMemory { (size_t)(memoryTracer.usedMemory / iterations) };
                double usedAllocs = (double)memoryTracer.usedAllocs / iterations;

                double percentDifference = (duration.seconds - projected.seconds) / projected.seconds * 100;

                std::cout 
                    << "pop count: " << std::setw(9) << count
                    << " time: " << std::setw(8) << duration << " / run"
                    << " (expected " << std::setw(8) << Seconds(expected * count) 
                    << " " << std::setw(4) << (int)percentDifference << "%)"
                    << "  iteration(s): " << std::setw(4) << iterations
                    << "  memory: " << std::setw(8) << usedMemory << " "
                    << std::setw(3) << (int)usedAllocs << " allocation(s)"
                    << std::endl;
                expected = benchDuration / count;
                memoryTracer.enter();
            });
        }
        return static_cast<This&>(*this);
    }
};

template <typename T>
struct PriorityQueuePopBenchmark : public PriorityQueueBenchmark<T, PriorityQueuePopBenchmark<T>> {
    T accumulator;
    void before (size_t i, PriorityQueue<T>& queue, const std::vector<T>& data) {
        // std::cout << data.size() << " elements\n";
        for (const auto& element : data) {
            queue.push(element);
        }
        // queue.debugOnSwap([](const PriorityQueue<T>& queue) {
        //     std::cout << queue << '\n';
        // });
        // accumulator += queue.size();
        // std::cout << queue << '\n';
    }
    void run (size_t i, PriorityQueue<T>& queue, const std::vector<T>& data) {
        while (!queue.empty()) {
            queue.pop();
        }
        accumulator += queue.size();
        // std::cout << queue << '\n';
    }
    void info () {
        std::cout << accumulator << '\n';
    }
};

int main () {
    std::cout << "Programmer: Seiji Emery\n"
              << "Programmer's id: M00202623\n"
              << "File: " __FILE__ "\n\n";

    PriorityQueuePopBenchmark<double>()
        .runSuite({
            { 1,  1000 }, 
            { 10, 1000 }, 
            { 100, 1000 },
            { 1000, 1000 },
            { 10000, 100 },
            { 100000, 5 },
            { 1000000, 2 },
            { 10000000, 1 },
            // { 100000000, 1 },
        })
        .info();
}



