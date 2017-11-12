// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// dvc_version_3.cpp
//
// Implements a very fast, and highly modular / extensible DVC parser. Also implements a bunch of 
// tests / benchmarks for default configurations of said parser(s) to meet assignment 8's part 1 / 2 specs.
//
// remote source: https://github.com/SeijiEmery/comp220/blob/master/assignment_08/src/dvc_version_3.cpp
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

#include "AssociativeArray.h"
#include "DynamicArray.h"



//
// Utilities
//

typedef size_t hash_t;

// Simple low overhead, no-copying slicable string implementation.
// (replacement for std::string / const char*; lifetime of referenced memory must be exceed that of all slices...)
template <typename T>
struct Slice {
private:
    T _start; size_t _size;
public:
    Slice () : Slice(nullptr, 0) {}
    Slice (T start, size_t size) : _start(start), _size(size) {}
    Slice (const Slice&) = default;
    Slice& operator= (const Slice&) = default;
    operator bool () const { return _start != nullptr; }
    T start () const { return _start; } 
    T end   () const { return _start + _size; }
    size_t size () const { return _size; }
    std::string str () const { return std::string(_start, _size); }
};

// Utility macro: assembles a 32-bit integer out of 4 characters / bytes.
// Assumes little endian, won't work on PPC / ARM (would just need to swap order).
// This is useful b/c we can replace strcmp() for very small, fixed strings
// (eg. "Fall ", "Spring "), and it's usable in a switch statement.
#define PACK_STR_4(a,b,c,d) \
    (((uint32_t)d << 24) | ((uint32_t)c << 16) | ((uint32_t)b << 8) | ((uint32_t)a))

// Parses a string that is assumed / required to be a 4-digit integer literal.
// Faster than atoi - this saved me about 1ms per run (out of ~6ms on 70k files), no joke.
size_t _4atoi (const char* str) {
    assert(!(str[0] < '0' || str[0] > '9' ||
        str[1] < '0' || str[1] > '9' ||
        str[2] < '0' || str[2] > '9' ||
        str[3] < '0' || str[3] > '9'));
    
    uint32_t s = *((uint32_t*)str);
    return (size_t)(
        (((s >> 0) & 0xff) - '0') * 1000 +
        (((s >> 8) & 0xff) - '0') * 100 +
        (((s >> 16) & 0xff) - '0') * 10 +
        (((s >> 24) & 0xff) - '0') * 1
    );
}
void unittest_4atoi () {
    // std::cout << _4atoi("01234") << '\n';
    // std::cout << _4atoi("8942") << '\n';
    assert(_4atoi("01234") == 123);
    assert(_4atoi("8942") == 8942);
}

//
// Parsing algorithm (optimized to only produce hash, etc)
//

struct ParseResult {
    std::string subject;
    int         section;
    size_t      hash;
    const char* line;

    friend std::ostream& operator<< (std::ostream& os, ParseResult& result) {
        return os << result.subject << "-" << result.section << " (hash " << result.hash << ")";
    }
};

bool parse (const char* line, ParseResult& result) {
    result.line = line;
    size_t semester = 0;
    switch (((uint32_t*)line)[0]) {
        case PACK_STR_4('S','p','r','i'): assert((((uint32_t*)line)[1] & 0x00FFFFFF) == PACK_STR_4('n','g',' ','\0')); line += 7; semester = 0; break;
        case PACK_STR_4('S','u','m','m'): assert((((uint32_t*)line)[1] & 0x00FFFFFF) == PACK_STR_4('e','r',' ','\0')); line += 7; semester = 1; break;
        case PACK_STR_4('F','a','l','l'): assert(line[4] == ' ');                                                         line += 5; semester = 2; break;
        case PACK_STR_4('W','i','n','t'): assert((((uint32_t*)line)[1] & 0x00FFFFFF) == PACK_STR_4('e','r',' ','\0')); line += 7; semester = 3; break;
        default: return false;
    }
    assert(isnumber(line[0]) && line[4] == '\t');
    result.hash = semester | (((_4atoi(line) - 2000) & 31) << 2);
    line += 5;

    assert(isnumber(line[0]) && line[4] == '\t');
    size_t code = _4atoi(line);
    result.hash |= (code << 8);
    line += 5;

    assert(isupper(line[0]));
    const char* end = strchr(line, '-');
    assert(end != nullptr && end != line);
    result.subject = { line, (size_t)(end - line) };
    result.section = atoi(end+1);
    return true;
}


int main (int argc, const char** argv) {
    unittest_4atoi();
    //Bitset::unittest();
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

    typedef AssociativeArray<int, int>              SectionCount;
    typedef AssociativeArray<std::string, SectionCount>     SubjectDict;
    SubjectDict subjects;

    // Load file
    std::ifstream file { path };
    if (!file) {
        std::cerr << "Could not load '" << path << "'" << std::endl;
        exit(-1);
    } else {
        std::cout << "Loaded file '" << path << "'" << std::endl;
    }

    std::string line;
    ParseResult result;
    while (getline(file, line)) {
        if (parse(line.c_str(), result)) {
            // std::cout << result << " (from " << line << ")\n";

            // Nice, behavior of associative array lets us do this:
            subjects[result.subject][result.section]++;
        }
    }

    // And since I implemented iterators using std::pair<K,V>:
    for (const auto& subject : subjects) {
        std::cout 
            << subject.first << ", " 
            << subject.second.size() << " course(s)\n";

        for (const auto& section : subject.second) {
            std::cout 
                << "    " << subject.first << '-' << section.first 
                << ", " << section.second << " section(s)\n";
        }
    }
    return 0;
}
