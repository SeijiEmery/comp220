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
    double elapsed () const { return (double)(startTime - endTime) / CLOCKS_PER_SEC; }

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
    void runSuite (std::initializer_list<std::pair<size_t, size_t>> counts) {
        for (const auto& pair : counts) {
            size_t count = pair.first, iterations = pair.second;
            srand(time(nullptr));
            generate(count, [](){ return static_cast<T>(rand() % 2048 - 1024); });
            run(iterations, [&](size_t iterations, double benchDuration /* seconds */) {
                std::cout << "Ran " << iterations << " iterations, avg " << benchDuration << " for " << count << " element(s)\n";
            });
        }
    }
};

template <typename T>
struct PriorityQueuePushBenchmark : public PriorityQueueBenchmark<T, PriorityQueuePushBenchmark<T>> {
    void run (size_t i, PriorityQueue<T>& queue, const std::vector<T>& data) {
        // std::cout << data.size() << " elements\n";
        for (const auto& element : data) {
            queue.push(element);
        }
        // std::cout << queue << '\n';
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
        });
}
