// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// DvcScheduleCheck.cpp
//
// Loads and searched DVC data for courses with conflicting section numbers
// (each section number for a given semester is supposed to be unique)
//
#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <cstring>
#include <map>

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
// DVC Date handling
//

enum class Season : uint8_t {
    SPRING = 0x0, SUMMER = 0x1, FALL = 0x2, WINTER = 0x3, INVALID = 0xFF
};
std::ostream& operator<< (std::ostream& os, Season season) {
    switch (season) {
        case Season::SPRING: return os << "Spring";
        case Season::SUMMER: return os << "Summer";
        case Season::FALL:   return os << "Fall";
        case Season::WINTER: return os << "Winter";
        default:             return os << "INVALID";
    }
}
// Parse season from string (must match exactly); returns INVALID on error. 
Season parseSeasonDVC (char*& str) {
    switch (reinterpret_cast<const uint32_t*>(str)[0]) {
        case PACK_STR_4('S','p','r','i'): assert((((uint32_t*)str)[1] & 0x0000FFFF) == PACK_STR_4('n','g','\0','\0')); str += 6; return Season::SPRING;
        case PACK_STR_4('S','u','m','m'): assert((((uint32_t*)str)[1] & 0x0000FFFF) == PACK_STR_4('e','r','\0','\0')); str += 6; return Season::SUMMER;
        case PACK_STR_4('F','a','l','l'): str += 4; return Season::FALL;
        case PACK_STR_4('W','i','n','t'): assert((((uint32_t*)str)[1] & 0x0000FFFF) == PACK_STR_4('e','r','\0','\0')); str += 7; return Season::WINTER;
        default: return Season::INVALID;
    }
}

class Date {
    uint8_t hash;
public:
    static constexpr int MIN_YEAR = 2000;
    static constexpr int MAX_YEAR = 2000 + (1 << (sizeof(hash) * 8 - 2)) - 1;

    Date () : hash(0) { assert(season() == Season::SPRING && year() == 2000); }
    Date (Season season, int year)
        : hash(static_cast<decltype(hash)>(season) | static_cast<decltype(hash)>((year - 2000) << 2))
    {
        if (year < MIN_YEAR || year > MAX_YEAR) {
            std::cerr << "Year exceeds bounds: " << year << " [" << MIN_YEAR << ", " << MAX_YEAR << "]" << std::endl;
            exit(-1);
        }
        assert(this->season() == season);
        assert(this->year() == year);
    }
    Date (const Date&)              = default;
    Date& operator= (const Date&)   = default;

    Season season () const { return static_cast<Season>(hash & 0x3);    }
    int    year   () const { return static_cast<int>(hash >> 2) + 2000; }

    friend bool operator== (const Date& a, const Date& b) { return a.hash == b.hash; }
    friend bool operator!= (const Date& a, const Date& b) { return a.hash != b.hash; }
    friend bool operator<= (const Date& a, const Date& b) { return a.hash <= b.hash; }
    friend bool operator>= (const Date& a, const Date& b) { return a.hash >= b.hash; }
    friend bool operator<  (const Date& a, const Date& b) { return a.hash < b.hash; }
    friend bool operator>  (const Date& a, const Date& b) { return a.hash > b.hash; }

    friend std::ostream& operator<< (std::ostream& os, const Date& date) {
        return os << date.season() << ' ' << date.year();
    }
};


//
// Parser
//

struct ParseResult {
    Date        date;
    uint16_t    section;
    const char* course;         // course string (including course #)
    const char* courseNumber;   // aliased pointer to course number (only)
    const char* instructor;
    const char* details;
};

bool parse (const char* file, size_t line_num, char* line, ParseResult& result) {
    #define require(context, expr) if (!(expr)) { /*warn(std::cerr) << "PARSING ERROR (" \
            << context << ", " __FILE__ ":" << __LINE__ << ") in " \
            << file << ':' << line_num << ", at '" << line << "')";*/ return false; }

    Season season = parseSeasonDVC(line);
    require("expected season", season != Season::INVALID);
    require("expected year", line[0] == ' ' && isnumber(line[1]) && line[5] == '\t');
    result.date = Date(season, _4atoi(&line[1]));
    line += 6;

    require("expected section", isnumber(line[0]) && line[4] == '\t');
    result.section = static_cast<decltype(result.section)>(_4atoi(line));
    line += 5;

    result.course = line; line = strchr(line, '-');
    require("expected course", isupper(result.course[0]) && line[0] == '-');
    ++line;

    result.courseNumber = &line[1]; line = strchr(line, '\t');
    require("expected course number", isnumber(result.courseNumber[0]) && *line == '\t');
    *line++ = '\0';

    result.instructor = line; line = strchr(line, '\t');
    require("expected instructor", isupper(result.instructor[0]) && line[0] == '\t');
    *line++ = '\0';

    result.details = line;
    return true;
    #undef require
}

template <typename F>
void parseDvc (const char* filePath, const F& callback) {
    std::ifstream file { filePath };
    if (!file) {
        warn(std::cerr) << "Could not load '" << filePath << "'"; std::cerr.flush();
        exit(-1);
    } else {
        report() << "Loaded file '" << filePath << "'. Parsing...";
    }
    ParseResult result;

    std::string line;
    for (size_t lineNum = 0; getline(file, line); ++lineNum) {
        if (parse(filePath, lineNum, &line[0], result)) {
            callback(result, lineNum, line);
        }
    }
}
template <typename F>
void parseDvc (int argc, const char** argv, const F& callback) {
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
    parseDvc(path, callback);
}

//
// Program implementation
//

int main (int argc, const char** argv) {
    unittest_4atoi();
    std::cout << "Programmer: Seiji Emery\n"
              << "Programmer's id: M00202623\n"
              << "File: " __FILE__ "\n\n";

    typedef std::map<uint16_t,  std::string> CoursesBySection;
    typedef std::map<Date,      CoursesBySection>   CoursesByTerm;
    CoursesByTerm courses;

    size_t numConflicts = 0;
    parseDvc(argc, argv, [&](const ParseResult& result, size_t lineNum, const std::string& line){
        auto& byTerm = courses[result.date];
        auto  entry  = byTerm.find(result.section);
        if (entry != byTerm.end() && entry->second != result.course) {
            warn() << "Course conflict: " << result.course << " (" << result.section << ") conflicts with "
                << entry->second << " in " << result.date;
            ++numConflicts;
        } else {
            byTerm[result.section] = result.course;
        }
    });
    if (numConflicts != 0) {
        warn() << numConflicts << " conflict(s)";
    } else {
        report() << "no conflicts";
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