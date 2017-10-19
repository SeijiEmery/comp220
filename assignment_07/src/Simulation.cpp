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

#include <iostream> // std::cout, std::cerr
#include <iomanip>  // std::setw
#include <fstream>  // std::ifstream
#include <string>   // std::string
using namespace std;

#include <cstdlib>
#include <cmath>
#include "Queue.h"
#include "DynamicArray.h"

struct ServerConfig {
    size_t serverCount    = 0;  // number of servers
    double arrivalRate    = 0;  // customer arrival rate
    size_t maxQueueLength = 0;  // maximum wait queue length
    size_t minServiceTime = 0;  // min service time, in minutes
    size_t maxServiceTime = 0;  // max service time, in minutes
    size_t arrivalEndTime = 0;  // time at which customers stop arriving, in minutes

    ServerConfig () {}

    friend std::istream& operator>> (std::istream& is, ServerConfig& config) {
        return is 
            >> config.serverCount
            >> config.arrivalRate
            >> config.maxQueueLength
            >> config.minServiceTime
            >> config.maxServiceTime
            >> config.arrivalEndTime;
        return is;
    }

    friend std::ostream& operator<< (std::ostream& os, const ServerConfig& config) {
        constexpr size_t indent = 2;
        return os << "number of servers: " << std::setw(indent+4) << config.serverCount << '\n'
            << "customer arrival rate: " << std::setw(indent) << config.arrivalRate 
            << " per minute, for " << config.arrivalEndTime << " minutes" << '\n'
            << "maximum queue length: " << std::setw(indent+1) << config.maxQueueLength << '\n'
            << "minimum service time: " << std::setw(indent+1) << config.minServiceTime << '\n'
            << "maximum service time: " << std::setw(indent+1) << config.maxServiceTime << '\n';
    }
    void loadFromFile (const char* path) {
        std::ifstream file { path };
        if (!file) { 
            std::cerr << "Could not load / locate '" << path << "'\n"; 
            exit(-1); 
        }
        if (!(file >> *this)) {
            std::cerr << "Failed to parse config file '" << path << "'!\n" << *this << "\n"; 
            exit(-1);
        }
    }
    void loadFromStream (std::istream& is) {
        if (!(is >> *this)) {
            std::cerr << "Failed to load config! Current values:\n" << *this << "\n";
            exit(-1);
        }
    }

    ServerConfig (int argc, const char** argv) {
        const char* configPath = "simulation.txt";
        switch (argc) {
            case 1: break;
            case 2: 
                if (strcmp(argv[1], "setup") == 0) {
                    ofstream file { configPath }; file << "4\n2.5\n8\n3\n10\n50" << std::endl;
                } else {
                    configPath = argv[1];
                }
                break;
            default: std::cerr << "usage: " << argv[0] << " [<simulation.txt>] | setup\n"; exit(-1);
        }
        loadFromFile(configPath);
    }
};

struct Customer {
private:
    static char nextId;
public:
    char   id;
    size_t arrivalTime;
    size_t serviceEndTime;

    Customer () : id('#'), arrivalTime(0), serviceEndTime(0) {}
    Customer (size_t arrivalTime, size_t serviceEndTime)
        : id(nextId = ((nextId+1 - 'A') % 26) + 'A'),
          arrivalTime(arrivalTime), serviceEndTime(serviceEndTime) 
    {}
    Customer (const Customer&) = default;
    Customer& operator= (const Customer&) = default;
    friend std::ostream& operator << (std::ostream& os, const Customer& customer) {
        return os << customer.id;
    }
};
char Customer::nextId = 'Z';

class Server {
    bool        isBusy = false;
    Customer    customer;
public:
    bool busy () const { return isBusy; }
    void simulate (size_t time) {
        isBusy = time < customer.serviceEndTime;
    }
    void serve (Customer customer_, size_t currentTime, size_t serviceDuration) {
        customer = customer_;
        customer.serviceEndTime = currentTime + serviceDuration;
        simulate(currentTime);
    }
    friend std::ostream& operator << (std::ostream& os, const Server& server) {
        return os << (server.busy() ? server.customer.id : '-');
    }
};


// Slightly extended version of DynamicArray that adds iterator support and a handful of 
// other features. Since we're only using this to store servers, we don't need to resize 
// etc – as such this array type is super minimalistic (b/c KISS).
//
template <typename T>
class FixedArray : public DynamicArray<T> {
    size_t _size = 0;
public:
    FixedArray (size_t size, const T& value) { fill(size, value); }
    ~FixedArray () {}

    size_t size () const { return _size; }

    void fill (size_t n, const T& value) {
        _size = n;
        while (n --> 0) {
            (*this)[n] = value;
        }
    }

    T* begin () { return &(*this)[0]; }
    T* end   () { return &(*this)[size()]; }

    const T* begin () const { return &(*this)[0]; }
    const T* end   () const { return &(*this)[size()]; }

    const T* cbegin () const { return begin(); }
    const T* cend   () const { return end(); }
};

// Given an average event rate and probability threshold on [0, 1], calculates the number of events 
// that would have occured per unit time
// from https://gist.github.com/SeijiEmery/76aa7637e7ce5b38bf5e133d721978f6
//
size_t poisson_k (double rate, double threshold) {
    double p = exp(-rate);
    size_t k = 0;
    while (threshold > p && k < 100) {
        threshold -= p;
        p *= rate / ++k;
    }
    return k;
}
template <typename T> T randUniform () {
    return static_cast<T>(rand()) / RAND_MAX;
}
template <typename T> T randRange (T min, T max) {
    return randUniform<T>() * (max - min) + min;
}

class Simulation {
    ServerConfig          config;
    FixedArray<Server>    servers;
    Queue<Customer>       waitQueue;
    size_t                currentTime = 0;
    bool                  isRunning = true;
public:
    Simulation (int argc, const char** argv)
        : config(argc, argv)
        , servers(config.serverCount, Server())
    {
        assert(servers.size() == config.serverCount);
        assert(servers.capacity() >= servers.size());
    }
    int run () {
        std::cout << config << '\n';
        bool tminus = false;
        while (1) {
            simulateStep();
            display();
            if (running()) {
                std::cout << "Press ENTER to continue...\n\n";
            } else {
                std::cout << "Done!\n";
                return 0;
            }   
        }
        return 0;
    }
private:
    bool running () const {
        return isRunning;
    }
    void display () {
        std::cout << "Time: " << currentTime << '\n';
        std::cout << "-----------------------------\n";
        std::cout << "server now-serving wait-queue\n";
        std::cout << "------ ----------- ----------\n";

        size_t i = 0;
        for (const auto& server : servers) {
            std::cout << std::setw(3) << i << std::setw(7) << server;
            if (i++ == 0) {
                std::cout << std::setw(12);
                for (const auto& customer : waitQueue) {
                    std::cout << customer;
                }
            }
            std::cout << '\n';
        }
        std::cout << "-----------------------------\n";
    }
    void simulateStep () {
        for (auto& server : servers) {
            server.simulate(currentTime);
        }
        if (currentTime < config.arrivalEndTime) {
            // Create new arrivals and push to waitQueue
            size_t arrivals = poisson_k(config.arrivalRate, randUniform<double>());
            while (arrivals-- && waitQueue.size() < config.maxQueueLength) {
                waitQueue.push(Customer(currentTime, 0)); 
            }
        }

        if (!waitQueue.empty()) {
            // Move any pending customers to non-busy servers
            for (auto& server : servers) {
                if (!server.busy()) {
                    server.serve(waitQueue.front(), currentTime,
                        randRange(static_cast<double>(config.minServiceTime), static_cast<double>(config.maxServiceTime)));
                    waitQueue.pop();
                    if (waitQueue.empty()) break;
                }
            }
        }
        size_t busyServers = 0;
        for (const auto& server : servers) {
            if (server.busy()) {
                ++busyServers;
            }
        }

        std::cout << "busy servers: " << busyServers << '\n';
        isRunning = currentTime < config.arrivalEndTime || busyServers > 0;

        // Advance current time
        ++currentTime;
    }
};


int main (int argc, const char** argv) {
    std::cout << "Programmer:       Seiji Emery\n";
    std::cout << "Programmer's ID:  M00202623\n";
    std::cout << "File:             " << __FILE__ << '\n' << std::endl;
    return Simulation(argc, argv).run();
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
