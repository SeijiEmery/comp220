// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// DvcSchedule10.cpp
//
// Hashtable-based DVC parser, used for both duplicate checking + counting.
//
// Note: this is fast, but is not nearly as fast / efficient as my other versions,
// which were extremely optimized to use eg. perfect hashing (based on characteristics
// of the data set) and a bitset for duplicate removal; using a hashtable is not
// nearly as efficient, though it is much more general.
// 
// Specifically, this uses ~15x as much memory and runs ~2.5x slower than DVC-4;
// it is significantly slower than DVC-8, which brought further improvements / optimizations
// and is highly modular.
//
// remote source: https://github.com/SeijiEmery/comp220/blob/master/assignment_10/src/DvcSchedule10.cpp
//
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <type_traits>
#include <ctime>
#include <cmath>
#include <vector>

#include "HashTable.h"

//
// Utilities
//

#define SET_COLOR(code) "\033[" code "m"
#define CLEAR_COLOR SET_COLOR("0")
#define SET_CYAN    SET_COLOR("36;1")
#define SET_RED     SET_COLOR("31;1")
#define SET_GREEN   SET_COLOR("32;1")
#define SET_YELLOW  SET_COLOR("33;1")
#define SET_BLUE    SET_COLOR("34;1")
#define SET_PINK    SET_COLOR("35;1")

struct LineWriter {
    std::ostream& os;
    bool shouldClearColor;
    LineWriter (std::ostream& os, const char* startColor = nullptr) : 
        os(os), shouldClearColor(startColor != nullptr)
    {
        if (startColor) { os << startColor; }
    }
    template <typename T>
    LineWriter& operator<< (const T& other) {
        return os << other, *this;
    }
    ~LineWriter () {
        if (shouldClearColor) { os << CLEAR_COLOR "\n"; }
        else { os << '\n'; }
    }
};
LineWriter writeln  (std::ostream& os, const char* color = nullptr) { return LineWriter(os, color); }
LineWriter writeln  (const char* color = nullptr) { return LineWriter(std::cout, color); }
LineWriter report (std::ostream& os = std::cout) { return writeln(os, SET_CYAN); }
LineWriter warn   (std::ostream& os = std::cout) { return writeln(os, SET_RED); }
LineWriter info   (std::ostream& os = std::cout) { return writeln(os, SET_GREEN); }


// Utility macro: assembles a 32-bit integer out of 4 characters / bytes.
// Assumes little endian, won't work on PPC / ARM (would just need to swap order).
// This is useful b/c we can replace strcmp() for very small, fixed strings
// (eg. "Fall ", "Spring "), and it's usable in a switch statement.
#define PACK_STR_4(a,b,c,d) \
    (((uint32_t)d << 24) | ((uint32_t)c << 16) | ((uint32_t)b << 8) | ((uint32_t)a))

//
// Main program
//

int main (int argc, const char** argv) {
    std::cout << "Programmer: Seiji Emery\n"
              << "Programmer's id: M00202623\n"
              << "File: " __FILE__ "\n\n";

    // Get path from program arguments
    const char* path = "dvc-schedule.txt";
    switch (argc) {
        case 1: break;
        case 2: path = argv[0]; break;
        default: {
            std::cerr << "usage: " << argv[0] << " [path-to-dvc-schedule.txt]" << std::endl;
            exit(-1);
        }
    }

    // Load file
    std::ifstream file { path };
    if (!file) {
        std::cerr << "Could not load '" << path << "'" << std::endl;
        exit(-1);
    } else {
        std::cout << "Loaded file '" << path << "'" << std::endl;
    }

    auto duplicates   = make_hashtable<std::string, bool>(std::hash<std::string>{});
    auto subjects     = make_hashtable<std::string, size_t>(std::hash<std::string>{});

    // Parse lines 2
    std::string line;
    std::string subject;
    while (getline(file, line)) {
        // Skip duplicate lines
        if (!duplicates.insert(line, true)) {
            // warn() << "duplicate line " << line;
            continue;
        }

        // Filter valid lines only (this is a very unsafe, albeit fast way to do this)
        const char* s = line.c_str();
        switch (((uint32_t*)s)[0]) {
            case PACK_STR_4('S','p','r','i'): break;
            case PACK_STR_4('S','u','m','m'): break;
            case PACK_STR_4('F','a','l','l'): break;
            case PACK_STR_4('W','i','n','t'): break;
            default: 
                // warn() << "skipping line " << line; 
                continue;
        }

        // Skip season + section fields
        const char* section = strchr(s, '\t') + 1;    assert(section != s + 1);

        // Get subject string (once again, unsafe / fast)
        const char* subj = strchr(section, '\t') + 1; assert(subj != section + 1);
        const char* end  = strchr(subj, '-');         assert(subj != end);

        // Count subject
        subject.assign(subj, static_cast<size_t>(end - subj));
        subjects[subject] += 1;
    }

    // Fetch + sort results:
    typedef std::pair<std::string, size_t> SubjectKV;
    std::vector<SubjectKV> results;
    results.reserve(subjects.size());
    for (auto& kv : subjects) {
        results.push_back(kv);
    }
    std::sort(results.begin(), results.end(), [](const SubjectKV& a, const SubjectKV& b) {
        return a.first < b.first;
    });

    for (const auto& subject : results) {
        writeln() << subject.first << ", " << subject.second << " section(s)";
    }
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
