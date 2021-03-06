// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// DvcSchedule5.cpp
//
// Big data assignment part 1. Or parts 1-N, as it were – what do you expect,
// for me to go back and write a slow version? B/c it's actually much simpler 
// to use a bitset for duplicate removal...
//
// This program reads a course listing file in the working directory (dvc-schedule.txt),
// calculates the number of unique class sections for each subject, and displays those
// sorted alphabetically.
//
// Program does not do any unnecessary work (eg. fully parsing + storing all fields), and does not
// validate input, which is assumed to be machine generated and to follow very specific formatting rules.
// If input does not follow these rules, the program will crash (but will crash early using 
// asserts(), etc; if compiled without asserts in eg. release (and given bad input) behavior is undefined.
// Any parser built on strtok() (this is not, but techniques are similar) would be as fragile + pedantic 
// (but fast) though.
//
// Program is also fast, as I've "cheated" and just built a fully optimized version (I discovered a neat
// way to uniquely hash the data entires, which means duplicates can be removed using a trivial hashset;
// I also implemented a bitset to cut down on memory usage).
//
// All requirements have been followed: output is correct, no STL containers were used. The DynamicArray
// from assignment 3 is the main, and only real data structure used (though I did layer a BitSet on top of that;
// its implementation is trivial and leverages DynamicArray features). The program does print some additional 
// info at the end: # of parsed fields and duplicates, and some performance / profiling info for memory usage 
// (all allocations / deallocations), and the time it took to run.
//
//
// Remote source / version history:
// https://github.com/SeijiEmery/comp220/blob/master/assignment_04/src/DvcSchedule4.cpp
//
// Compile switches / flags:
//  –D LOG_DUPLICATES: if enabled, prints debugging info to a file called duplicates.cpp.txt
//  -D NO_MEM_DEBUG: disables optional memory + time profiling
//

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
using namespace std;

#include <cstring>

// Our dynamic array impl from assignment 3.
#include "DynamicArray.h"


// Generic-ish string hashing fuction; works decently for strings of uppercase characters (['A','Z'])
// Used to implement a hashtable for subject strings.
size_t hashString (const uint8_t* key, size_t len) {
    size_t hash = 0;
    for (; len --> 0; ++key) {
        hash *= 26;
        hash += (*key - 'A');
    }
    return hash;
}

// Bitset data structure, used to implement a simple hashset for duplicate element removal
// (can use perfect hashing for this data set due to its unique properties).
class Bitset {
    DynamicArray<size_t> array;
    enum { BITS = 4 * sizeof(size_t) };
public:
    Bitset (size_t count) : array(count / BITS + 1) {}
    void set   (size_t i) { array[i / BITS] |=  (1 << (i % BITS)); }
    void clear (size_t i) { array[i / BITS] &= ~(1 << (i % BITS)); }
    bool get   (size_t i) { return array[i / BITS] & (1 << (i % BITS)); }
    static void unittest ();
};

// Small data structure, used to store a subject name + count pair.
struct Subject { 
    std::string name; 
    size_t count = 0;
    size_t hashid = 0;

    Subject () = default;
    Subject (decltype(name) name, decltype(count) count)
        : name(name), count(count) {}
};

// Utility macro: assembles a 32-bit integer out of 4 characters / bytes.
// Assumes little endian, won't work on PPC / ARM (would just need to swap order).
// This is useful b/c we can replace strcmp() for very small, fixed strings
// (eg. "Fall ", "Spring "), and it's usable in a switch statement.
#define PACK_STR_4(a,b,c,d) \
    (((uint32_t)d << 24) | ((uint32_t)c << 16) | ((uint32_t)b << 8) | ((uint32_t)a))


// Main program.
int main (int argc, const char** argv) {
    Bitset::unittest();
    std::cout << "Programmer: Seiji Emery\n"
              << "Programmer's id: M00202623\n"
              << "File: " __FILE__ "\n\n";

    // Get path from program arguments
    const char* path = nullptr;
    switch (argc) {
        case 1: path = "dvc-schedule.txt"; break;
        case 2: path = argv[0]; break;
        default: {
            std::cerr << "usage: " << argv[0] << " [path-to-dvc-schedule.txt]" << std::endl;
            exit(-1);
        }
    }

    // Load file, and check that successful
    ifstream file { path };
    if (!file) {
        std::cerr << "Could not open file: '" << path << "'" << std::endl;
        exit(-1);
    }

    // Array to store subject counts
    DynamicArray<Subject> subjects;
    size_t numsubjects = 0, linecount = 0;
    size_t hashcollisions = 0, hashcollisioncount = 0;
    size_t num_unique_primary_hashes = 0, num_unique_secondary_hashes = 0;

    // Bitset used to check for object duplicates. For this we use a perfect hashing algorithm, see bleow.
    Bitset hashset (1);

    // Some additional variables to trace number of duplicates / unique entries.
    // If the compiler flag -D LOG_DUPLICATES is defined, we'll also log all values (INSERT | DUPLICATE + line)
    // to an external log file for debugging. Is named duplicates.cpp.txt since I'm comparing the output against
    // a python program (for reference, since the output of that is provably correct).
    size_t dupcount = 0, uniquecount = 0;
    #ifdef LOG_DUPLICATES
        ofstream duplicate_log { "duplicates.cpp.txt" };
    #endif

    std::cout << "Parsing lines...";
    string line;
    while (getline(file, line)) {
        char* s = const_cast<char*>(line.c_str());

        // For each line, parse + assemble a unique 23-bit hash code:
        // 1. 2-bit code for semester (Spring = 0, Summer = 1, Fall = 2, Winter = 3)
        size_t semester = 0;

        switch (((uint32_t*)s)[0]) {
            case PACK_STR_4('S','p','r','i'): assert((((uint32_t*)s)[1] & 0x00FFFFFF) == PACK_STR_4('n','g',' ','\0')); s += 7; semester = 0; break;
            case PACK_STR_4('S','u','m','m'): assert((((uint32_t*)s)[1] & 0x00FFFFFF) == PACK_STR_4('e','r',' ','\0')); s += 7; semester = 1; break;
            case PACK_STR_4('F','a','l','l'): assert(s[4] == ' ');                                                      s += 5; semester = 2; break;
            case PACK_STR_4('W','i','n','t'): assert((((uint32_t*)s)[1] & 0x00FFFFFF) == PACK_STR_4('e','r',' ','\0')); s += 7; semester = 3; break;
            default: if (linecount == 0) { continue; } else { assert(0); }
        }

        // 2. 5-bit hash code for year, normalized to starting year (2000 = 0).
        //    This is sufficient to accomodate years in range [2000, 2032), and will fail / hash collide after that
        assert(isnumber(s[0]) && s[4] == '\t');
        size_t hash = semester | (((atoi(s) - 2000) & 31) << 2);
        s += 5;

        // 3. n-bit section # (4 digits => should be at most 14 bits, but can be whatever)
        assert(isnumber(s[0]) && s[4] == '\t');
        size_t code = atoi(s);
        hash |= (code << 8);
        s += 5;

        // We then filter duplicates by checking our hashset (implemented as a bitset): 
        // if bit has been set, it's a duplicate, if not, add it to the hash set and continue.
        if (hashset.get(hash)) {
            #ifdef LOG_DUPLICATES
                duplicate_log << "DUPLICATE: " << hash << " '" << line << "'\n";
            #endif
            ++dupcount;
            continue;
        } else {
            #ifdef LOG_DUPLICATES
                duplicate_log << "INSERTING: " << hash << " '" << line << "'\n";
            #endif
            ++uniquecount;
            hashset.set(hash);
        }

        // Insert subject into array. As an optimization, we treat subjects as a hashtable,
        // and use a string hash (different than the perfect hashing algorithm above) to 
        // determine its initial placement

        assert(isupper(s[0]));
        char* end = strchr(s, '-');
        assert(end != nullptr && end != s);
        *end = '\0';
        size_t str_hash = hashString((uint8_t*)s, (size_t)end - (size_t)s);

        #define HASHTABLE_SIZE (1024)


        // How this works: we insert into an initial position at subjects[str_hash] (index), 
        // modulo HASHTABLE_SIZE (ie. hashtable will never grow beyond HASHTABLE_SIZE).
        // If this slot is:
        //      empty     => insert into slot, set count 1
        //      matching  => increment count
        //      not matching => increment hash / index (modulo HASHTABLE_SIZE) and retry 
        //                      until we find a free / matching slot
        //

        str_hash %= HASHTABLE_SIZE;
        if (subjects[str_hash].count == 0) {
            ++num_unique_primary_hashes;
            subjects[str_hash].name = s;
            subjects[str_hash].hashid = str_hash;
        } else if (subjects[str_hash].name != s) {
            ++hashcollisions;
            // std::cout << "Hash collision! '" << s << "' (" << str_hash << "), '" 
            //     << subjects[str_hash].name << "' (" << subjects[str_hash].hashid << ")\n";
            do {
                str_hash = (str_hash + 1) % HASHTABLE_SIZE;
                ++hashcollisioncount;
            } while (subjects[str_hash].count != 0 && subjects[str_hash].name != s);

            if (subjects[str_hash].count == 0) {
                ++num_unique_secondary_hashes;
            }
            subjects[str_hash].name = s;
            subjects[str_hash].hashid = str_hash;
        }
        ++subjects[str_hash].count;
    end:
        // Update progress counter
        if (((++linecount) % 1024) == 0) {
            // std::cout << '.'; std::cout.flush();
        }
    }
    std::cout << '\n';

    int swapCount = 0;

    // Since using hashtable, we must condense items towards the front.
    // By iterating forward, we ensure that they remain partially ordered;
    // we also count the number of non-empty elements as well.
    numsubjects = 0;
    for (size_t i = 0, j = 0; i < HASHTABLE_SIZE; ++i) {
        if (subjects[i].count != 0) {
            if (i != numsubjects) {
                std::swap(subjects[i], subjects[numsubjects]);
                ++swapCount;
            }
            ++numsubjects;
        }
    }

    std::cout << "Sorting sections...";
    for (auto i = 1; i < numsubjects; ++i) {
        for (auto j = i; j < numsubjects; ++j) {
            if (subjects[i].name > subjects[j].name) {
                swap(subjects[i], subjects[j]);
                ++swapCount;
            }
        }
    }
    std::cout << '\n';    

    // Display sorted results
    size_t totalSections = 0;
    for (auto i = 1; i < numsubjects; ++i) {
        if (subjects[i].count) {
            std::cout << subjects[i].name << ", " << subjects[i].count << " sections\n";
        }
        totalSections += subjects[i].count;
    }
    std::cout << '\n';
    std::cout << "total: " << totalSections << " sections, " << numsubjects << " subjects\n";
    std::cout << "parsed " << uniquecount << " fields, removed " << dupcount << " duplicates\n";
    std::cout << "sorting used " << swapCount << " swaps\n";
    std::cout << "hashtable size: " << HASHTABLE_SIZE << " => " 
        << num_unique_primary_hashes << " primary hashes, "
        << num_unique_secondary_hashes << " secondary hashes\n";
    std::cout << "hashcollisions: " << hashcollisions
        << ", avg distance " << ((double)hashcollisioncount) / ((double)hashcollisions) 
        << ", avg per item " << ((double)hashcollisioncount) / ((double)uniquecount) << "\n";
}

// Simple unittests for bitset (assumes that our already tested DynamicArray works properly...)
void Bitset::unittest () {
    Bitset bitset (10);
    bitset.set(7);
    for (auto i = 0; i < 16; ++i) {
        assert(bitset.get(i) == (i == 7));
    }
    bitset.set(8);
    for (auto i = 0; i < 16; ++i) {
        assert(bitset.get(i) == (i == 7 || i == 8));
    }
    bitset.set(9);
    for (auto i = 0; i < 16; ++i) {
        assert(bitset.get(i) == (i == 7 || i == 8 || i == 9));
    }
    bitset.clear(9);
    bitset.clear(7);
    bitset.set(31);
    for (auto i = 0; i < 32; ++i) {
        assert(bitset.get(i) == (i == 8 || i == 31));
    }
    bitset.set(2417491);
    for (auto i = (2417491 / 8) * 8; i < (2417491 / 8 + 1) * 8; ++i) {
        assert(bitset.get(i) == (i == 2417491));
    }
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

static size_t g_num_allocations = 0;
static size_t g_num_frees       = 0;

static size_t g_allocated_mem = 0;
static size_t g_freed_mem = 0;

void* operator new (size_t size) throw(std::bad_alloc) {
    g_allocated_mem += size;
    ++g_num_allocations;
    size_t* mem = (size_t*)std::malloc(size + sizeof(size_t));
    if (!mem) {
        throw std::bad_alloc();
    }
    mem[0] = size;
    return (void*)(&mem[1]);
}
void operator delete (void* mem) throw() {
    auto ptr = &((size_t*)mem)[-1];
    g_freed_mem += ptr[0];
    ++g_num_frees;
    std::free(ptr);
}
struct PerfLogger {
    std::chrono::high_resolution_clock::time_point t0;

    PerfLogger () : t0(std::chrono::high_resolution_clock::now()) {}
    ~PerfLogger () {
        using namespace std::chrono;
        auto t1 = high_resolution_clock::now();
        std::cout << "\nUsed  memory: " << ((double)g_allocated_mem) * 1e-6 << " MB (" << g_num_allocations << " allocations)\n";
        std::cout << "Freed memory: "   << ((double)g_freed_mem) * 1e-6     << " MB (" << g_num_frees       << " deallocations)\n";
        std::cout << "Ran in " << duration_cast<duration<double>>(t1 - t0).count() * 1e3 << " ms\n";
    }
} g_perf_logger;

#endif // NO_MEM_DEBUG
