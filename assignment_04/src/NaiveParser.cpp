// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// DvcSchedule4.cpp
//

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
using namespace std;

#include "DynamicArray.h"

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
        std::cout << "Ran in " << duration_cast<duration<double>>(t1 - t0).count() << " seconds\n";
    }
} g_perf_logger;


#ifdef STD_BITSET
#include <vector>
class Bitset {
    std::vector<bool> values;
public:
    Bitset (size_t count) : values(count) {}
    void set   (size_t i) { if (i >= values.size()) values.resize(i+1); values[i] = true; }
    void clear (size_t i) { if (i >= values.size()) values.resize(i+1); values[i] = false; }
    bool get   (size_t i) { return i < values.size() ? values[i] : false; }
};
#else
class Bitset {
    DynamicArray<size_t> array;
    enum { BITS = 4 * sizeof(size_t) };
public:
    Bitset (size_t count) : array(count / BITS + 1) {}
    void set   (size_t i) { array[i / BITS] |=  (1 << (i % BITS)); }
    void clear (size_t i) { array[i / BITS] &= ~(1 << (i % BITS)); }
    bool get   (size_t i) { return array[i / BITS] & (1 << (i % BITS)); }
};
#endif

static void unittest_bitset () {
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

struct Subject { 
    std::string name; 
    size_t count = 0; 

    Subject () = default;
    Subject (decltype(name) name, decltype(count) count)
        : name(name), count(count) {}
};

#define PACK_STR_4(a,b,c,d) \
    (((uint32_t)d << 24) | ((uint32_t)c << 16) | ((uint32_t)b << 8) | ((uint32_t)a))

int main (int argc, const char** argv) {
    std::cout << "Programmer: Seiji Emery\n"
              << "Programmer's id: M00202623\n"
              << "File: " __FILE__ "\n\n";

    const char* path = nullptr;
    switch (argc) {
        case 1: path = "dvc-schedule.txt"; break;
        case 2: path = argv[0]; break;
        default: {
            std::cerr << "usage: " << argv[0] << " [path-to-dvc-schedule.txt]" << std::endl;
            exit(-1);
        }
    }

    ifstream file { path };
    if (!file) {
        std::cerr << "Could not open file: '" << path << "'" << std::endl;
        exit(-1);
    }

    DynamicArray<Subject> subjects;
    size_t numsubjects = 0, linecount = 0;
    Bitset hashset (1);

    size_t dupcount = 0, uniquecount = 0;
    #ifdef LOG_DUPLICATES
        ofstream duplicate_log { "duplicates.cpp.txt" };
    #endif

    std::cout << "Parsing lines...";
    string line;
    while (getline(file, line)) {
        char* s = const_cast<char*>(line.c_str());

        // For each line, parse + assemble a unique hash code:
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

        // 3. n-bit section # (4 digits => ~14 bits)
        assert(isnumber(s[0]) && s[4] == '\t');
        size_t code = atoi(s);
        hash |= (code << 8);
        s += 5;

        // We then filter duplicates by checking our hashset (implemented as a bitset): 
        // if bit has been set, it's a duplicate, if not, add it to the hash set and continue.
        if (hashset.get(hash)) {
            // duplicate_log << "DUPLICATE: " << hash << " '" << line << "'\n";
            ++dupcount;
            continue;
        } else {
            // duplicate_log << "INSERTING: " << hash << " '" << line << "'\n";
            ++uniquecount;
            hashset.set(hash);
        }

        // Compile-time switch: if compiled with -D SELECT_COURSES, groups by subject code,
        // if not, groups by course code (subject code + course number)
        #ifndef SELECT_COURSES
            #define TERMINAL '-'
        #else
            #define TERMINAL '\t'
        #endif

        assert(isupper(s[0]));
        char* tok = s;
        for (; *tok != '\0'; ++tok) {
            if (*tok == TERMINAL) {
                *tok = '\0';
                break;
            }
        }
        // std::cout << "SUBJECT: " << s << '\n';
        for (auto k = 0; k < numsubjects; ++k) {
            if (strcmp(subjects[k].name.c_str(), s) == 0) {
                ++subjects[k].count;
                goto end;
            }
        }
        subjects[numsubjects++] = Subject({ s }, 1);
        end:

        // Update progress counter
        if (((++linecount) % 1024) == 0) {
            std::cout << '.'; std::cout.flush();
        }
    }
    std::cout << '\n';


    // Sort elements (naive / bubble sort)
    std::cout << "Sorting sections...";
    for (auto i = 1; i < numsubjects; ++i) {
        for (auto j = i; j < numsubjects; ++j) {
            if (subjects[i].name > subjects[j].name) {
                swap(subjects[i], subjects[j]);
            }
        }
        // Update progress thingy...
        if ((i % (numsubjects / 32)) == 0) { 
            std::cout << '.'; std::cout.flush(); 
        }
    }
    std::cout << '\n';    

    // Display elements
    size_t totalSections = 0;
    for (auto i = 1; i < numsubjects; ++i) {
        if (subjects[i].count) {
            std::cout << subjects[i].name << ", " << subjects[i].count << " sections\n";
        }
        totalSections += subjects[i].count;
    }
    std::cout << '\n';
    std::cout << "total: " << totalSections << " sections\n";
    std::cout << "parsed " << uniquecount << " fields, removed " << dupcount << " duplicates\n";
}
