// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// TestKV.cpp
// Implements a set of interactive tests for AssociativeArray by implementing
// an interactive REPL w/ a simple regex-based DSL for manipulating a single
// AssociativeArray<string, string>.
//
// Remote source:
// https://github.com/SeijiEmery/comp220/tree/master/assignment_09/src/KVTest.hpp
//


#include <iostream>     // std::cout, std::cin
#include <sstream>      // std::stringstream
#include <string>       // std::string
#include <regex>        // std::regex, std::regex_search, std::smatch
#include <vector>       // std::pair
#include <utility>      // std::pair
#include <functional>   // std::function
#include <cstdlib>  // atoi, exit
#include "HashTable.h"


//
// Simple regex-based parser to implement a simple DSL / interpreter to test hashtable with
//

struct SimpleRegexParser {
    typedef std::function<void(const std::smatch&)> callback_t;
    typedef std::pair<std::regex, callback_t> case_t;
    std::vector<case_t> cases;
public:
    SimpleRegexParser () {}
    SimpleRegexParser (std::initializer_list<case_t> cases) 
        : cases(cases) {}

private:
    static std::string replace_whitespace (std::string input) {
        static std::regex r { "([ ]+)"};
        return std::regex_replace(input, r, "\\s*");
    }
    static std::string replace_group (std::string input) {
        static std::regex r { "\\{[^\\}]*\\}" };
        return std::regex_replace(input, r, "(\\w+)");
    }
    static std::string escape_chars (std::string input) {
        static std::regex r { "([\\.\\+\\=\\[\\]\\(\\)\\-\\?\\:])" };
        return std::regex_replace(input, r, ("\\$1"));
    }
    static std::string to_regex (std::string input) {
        return "^\\s*" + replace_group(replace_whitespace(escape_chars(input)));
    }
public:
    SimpleRegexParser& caseOf (const char* pattern, callback_t callback) {
        auto regex = to_regex(pattern);
        // info() << "Creating case from " << pattern;
        // info() << "Generated regex:   " << regex;
        cases.emplace_back(std::regex(regex), callback);
        return *this;
    }
    bool parse (std::string& line, std::smatch &match) const {
        for (auto& case_ : cases) {
            if (std::regex_search(line, match, case_.first)) {
                case_.second(match);
                line = match.suffix().str();
                return true;
            }
        }
        return false;
    }
    void parse (std::istream& is) const {
        std::string line;
        std::smatch match;
        while (is) {
            while (!getline(is, line));
            while (parse(line, match));
        }
    }
};

//
// Nicer I/O w/ colored text
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

//
// Functional programming stuff
//

//
// Convert iterators into ranges
//

template <typename It>
class RSequence {
    It begin, end;
public:
    RSequence (It begin, It end) : begin(begin), end(end) {}
    operator bool () const { return begin != end; }
    RSequence& operator++ () { return ++begin, *this; }
    auto operator* () const -> decltype(*begin) { return *begin; }
};
template <typename It>
auto sequence (It begin, It end) -> RSequence<It> { return { begin, end }; }


//
// Map
//

template <typename F, typename Range>
class RMap {
    F f;
    Range range;
public:
    RMap (F f, Range range) : f(f), range(range) {}
    operator bool () const { return bool(range); }
    RMap& operator++ () { return ++range, *this; }
    auto  operator*  () const -> decltype(f(*range)) { return f(*range); }
};
template <typename F, typename Range>
auto map (F f, Range range) -> RMap<F,Range> { return { f, range }; }

template <typename F, typename It>
auto map (F f, It begin, It end) -> decltype(map(f, sequence(begin, end))) {
    return map(f, sequence(begin, end));
}

//
// Filter
//

template <typename F, typename Range>
class RFilter {
    F f;
    Range range;

    void advanceFilter () { while (range && !f(*range)) { ++range; } }
public:
    RFilter (F f, Range range) : f(f), range(range) {}
    operator bool () const { return bool(range); }
    RFilter& operator++ () { ++range; advanceFilter(); return *this; }
    auto operator* () const -> decltype(*range) { return *range; }
};
template <typename F, typename Range>
auto filter (F f, Range range) -> RFilter<F,Range> { return { f, range }; }

template <typename F, typename It>
auto filter (F f, It begin, It end) -> decltype(filter(f, sequence(begin, end))) {
    return filter(f, sequence(begin, end));
}

//
// Reduce
//

template <typename F, typename Range, typename R>
auto reduce (F f, Range range, R first) -> R {
    for (; range; ++range) {
        first = f(first, *range);
    }
    return first;
}
template <typename F, typename Range>
auto reduce (F f, Range range) -> decltype(*range) {
    if (!range) return {};
    else {
        auto first = *range; ++range;
        for (; range; ++range) {
            first = f(first, *range);
        }
        return first;
    }
}

//
// Join (special case)
//

template <typename Range>
std::string join (std::string sep, Range range) {
    if (!range) return "";
    else {
        std::stringstream ss;
        ss << *range; ++range;
        for (; range; ++range) {
            ss << sep << *range;
        }
        return ss.str();
    }
}

//
// Take
//

template <typename Range>
struct RTake {
    size_t count;
    Range range;
public:
    RTake (size_t count, Range range) : count(count), range(range) {}
    operator bool () const { return count > 0 && range; }
    RTake& operator++ () { ++range; --count; return *this; }
    auto operator* () const -> decltype(*range) { return *range; }
};
template <typename Range>
auto take (size_t n, Range range) -> RTake<Range> {
    return { n, range };
}


int main () {
    typedef const std::smatch&  Match;
    typedef std::string         Key;
    typedef std::string         Value;
    
    auto array = make_hashtable<Key,Value>(std::hash<Key>{}, 8);
    typedef decltype(array)::KeyValue     KV;
    typedef decltype(array)::This         Dict;

    bool showFill = true;

    writeln();
    report() << "Starting main program";

    SimpleRegexParser()
        .caseOf("quit", [&](Match match) {
            warn() << "exiting";
            exit(0);
        })
        .caseOf(".|list|print|dump", [&](Match match) {
            if (array.size() == 0) {
                report() << "[]";
            } else {
                static const auto toString = [](const KV& kv) -> std::string { return kv.first + " = " + kv.second; };
                report() << "[ " << join(", ", map(toString, array.begin(), array.end())) << " ]";
            }
        })
        .caseOf("capacity", [&](Match match) {
            report() << array.capacity();
        })
        .caseOf("length|size", [&](Match match) {
            report() << array.size();
        })
        .caseOf("lf {}", [&](Match match) {
            double lf = atof(match[1].str().c_str());
            array.loadFactor(lf / 100.0);
            report() << "Set load factor = " << array.loadFactor(); 
        })
        .caseOf("lf", [&](Match match) {
            report() << "load factor = " << array.loadFactor();
        })
        .caseOf("show", [&](Match match) {
            array.each([&](size_t i, bool isSet, const std::pair<Key,Value>& kv) {
                if (isSet) { report() << i << ": " << kv.first << " => " << kv.second; }
                else       { report() << i << ": --"; }
            });
        })
        .caseOf("displayfill on", [&](Match match) {
            showFill = true;
        })
        .caseOf("displayfill off", [&](Match match) {
            showFill = false;
        })
        .caseOf("displayfill {} {}", [&](Match match) {
            auto a = atoi(match[1].str().c_str());
            auto b = atoi(match[2].str().c_str());
            if (b < a) { std::swap(a, b); }

            report() << "Filling range " << a << " to " << b;
            for (; a < b; ++a) {
                array[std::to_string(a)] = std::to_string(a);
                if (showFill) {
                    report() << "\033[2J\033[;H" << array;
                }
            }
        })
        .caseOf("display|info", [&](Match match) {
            report() << array;
        })
        .caseOf("fill {} {}", [&](Match match) {
            auto a = atoi(match[1].str().c_str());
            auto b = atoi(match[2].str().c_str());
            if (b < a) { std::swap(a, b); }

            report() << "Filling range " << a << " to " << b;
            for (; a < b; ++a) {
                array[std::to_string(a)] = std::to_string(a);
                if (showFill) {
                    report() << array;
                }
            }
        })
        .caseOf("resize {}", [&](Match match) {
            auto size = atoi(match[1].str().c_str());
            report() << "Resizing to " << size << " (current size " << array.size() << ")";
            array.resize(size);
        })
        .caseOf("clear", [&](Match match) {
            array.clear();
        })
        .caseOf("del {}", [&](Match match) {
            auto key = match[1].str();
            if (array.containsKey(key)) {
                report() << "deleted " << key;
                array.deleteKey(key);
            } else {
                warn() << "missing key " << key;
            }
        })
        .caseOf("{} = {}", [&](Match match) {
            auto key = match[1].str(), value = match[2].str();
            array[key] = value;
            report() << key << " = " << value;
        })
        .caseOf("{}", [&](Match match) {
            auto key = match[1].str();
            if (array.containsKey(key)) {
                report() << array[key];
            } else {
                warn() << "undefined";
            }
        })
        .parse(std::cin);
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
