// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// BetterSimulation.cpp
//
// Implements a simpler server simulation using our PriorityQueue implementation.
// Note: not a real server, not threadsafe, does not do anything or run in
// real time, etc.
//
// remote source: https://github.com/SeijiEmery/comp220/blob/master/assignment_11/src/BetterSimulation.cpp
//

#include <iostream> // std::cout, std::cerr
#include <iomanip>  // std::setw
#include <fstream>  // std::ifstream
#include <string>   // std::string
#include <vector>
using namespace std;

#include <cstdlib>
#include <cmath>
#include "PriorityQueue.h"

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
                    std::cout << "Generated default config file '" << configPath << "' in current directory\n";
                    std::cout << "Re-run with no arguments to run\n";
                    exit(0);
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
    static size_t nextOrd;
public:
    char   id;
    size_t order = 0;

    // Customer () : id('#') {}
    Customer ()
        : id(nextId = ((nextId+1 - 'A') % 26) + 'A')
        , order(nextOrd++)
    {}
    Customer (const Customer&) = default;
    Customer& operator= (const Customer&) = default;
    friend std::ostream& operator << (std::ostream& os, const Customer& customer) {
        return os << customer.id;
    }
    bool operator> (const Customer& other) { return order > other.order; }
    bool operator>= (const Customer& other) { return order >= other.order; }
};
char Customer::nextId = 'Z';
size_t Customer::nextOrd = 0;

class Server {
    bool        isBusy = false;
    Customer    customer;
public:
    bool busy () const { return isBusy; }
    void serve (Customer customer_) {
        customer = customer_;
        isBusy = true;
    }
    void endService () {
        isBusy = false;
    }
    friend std::ostream& operator << (std::ostream& os, const Server& server) {
        return os << (server.busy() ? server.customer.id : '-');
    }
};

struct ServiceEvent {
    size_t server;
    size_t timestamp;
public:
    ServiceEvent (decltype(server) server, decltype(timestamp) timestamp)
        : server(server), timestamp(timestamp) {}
    friend std::ostream& operator << (std::ostream& os, const ServiceEvent& event) {
        return os << "event { server = " << event.server << ", time = " << event.timestamp << " }";
    }

    bool operator> (const ServiceEvent& other) {
        return timestamp < other.timestamp;
    }
    bool operator>= (const ServiceEvent& other) {
        return timestamp <= other.timestamp;
    }
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
    ServerConfig                config;
    std::vector<Server>         servers;
    PriorityQueue<ServiceEvent> eventQueue;
    PriorityQueue<Customer>     waitQueue;
    size_t                  currentTime = 0;
    bool                    isRunning = true;
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
                std::cout << "Press ENTER to continue...\n";
                std::cin.get();
            } else {
                std::cout << "Done!\n";
                return 0;
            }   
        }
    }
private:
    bool running () const {
        return isRunning;
    }
    void display () {
        std::cout << "Time: " << currentTime << '\n';
        // std::cout << "Service queue: " << eventQueue << '\n';
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
        if (eventQueue.empty()) {
            std::cout << "No scheduled end-of-service events at this time\n";
        } else {
            std::cout << "Next scheduled end-of-service event in " 
                << (eventQueue.peek().timestamp - currentTime) << " minute(s)\n";
            std::cout << "End-of-service events: \n";

            for (auto temp = eventQueue; !temp.empty(); temp.pop()) {
                const auto& event = temp.peek();
                std::cout << "   " << servers[event.server] << " in " << (event.timestamp - currentTime) << " minute(s)\n";
            }
        }
        std::cout << "-----------------------------\n";
    }
    void simulateStep () {
        if (currentTime < config.arrivalEndTime) {
            // Create new arrivals and push to waitQueue
            size_t arrivals = poisson_k(config.arrivalRate, randUniform<double>());
            while (arrivals-- && waitQueue.size() < config.maxQueueLength) {
                waitQueue.push(Customer()); 
            }
        }

        // Process scheduled events
        while (!eventQueue.empty() && eventQueue.peek().timestamp == currentTime) {
            servers[eventQueue.peek().server].endService();
            eventQueue.pop();
        }

        // Dispatch new events, where possible
        for (size_t i = 0; !waitQueue.empty() && i < servers.size(); ++i) {
            if (!servers[i].busy()) {
                auto delay = randRange(
                    static_cast<double>(config.minServiceTime), 
                    static_cast<double>(config.maxServiceTime)
                );

                eventQueue.push({ i, currentTime + (size_t)delay });
                servers[i].serve(waitQueue.peek());
                waitQueue.pop();
            }
        }

        // Count busy servers
        size_t busyServers = 0;
        for (const auto& server : servers) {
            if (server.busy()) {
                ++busyServers;
            }
        }
        // std::cout << "busy servers: " << busyServers << '\n';
        isRunning = currentTime < config.arrivalEndTime || busyServers > 0;
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
