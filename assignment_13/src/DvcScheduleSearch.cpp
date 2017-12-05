// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// DvcScheduleSearch.cpp
//
// Loads DVC and provides an interactive way to search through courses.
//
// Uses ANSI text colors, so run this in a terminal (please).
//
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cassert>

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
// Parsing algorithm
//

struct ParseResult {
    std::string subject;
    std::string section;
    size_t      hash;
    const char* line;

    friend std::ostream& operator<< (std::ostream& os, ParseResult& result) {
        return os << result.subject << "-" << result.section << " (hash " << result.hash << ")";
    }
};

bool parse (const char* file, size_t line_num, const char* line, ParseResult& result) {
    #define require(context, expr) if (!(expr)) { warn(std::cerr) << "PARSING ERROR (" \
            << context << ", " __FILE__ ":" << __LINE__ << ") in " \
            << file << ':' << line_num << ", at '" << line << "')"; return false; }

    result.line = line;
    size_t semester = 0;
    switch (((uint32_t*)line)[0]) {
        case PACK_STR_4('S','p','r','i'): require("expected season", (((uint32_t*)line)[1] & 0x00FFFFFF) == PACK_STR_4('n','g',' ','\0')); line += 7; semester = 0; break;
        case PACK_STR_4('S','u','m','m'): require("expected season", (((uint32_t*)line)[1] & 0x00FFFFFF) == PACK_STR_4('e','r',' ','\0')); line += 7; semester = 1; break;
        case PACK_STR_4('F','a','l','l'): require("expected season", line[4] == ' ');                                                         line += 5; semester = 2; break;
        case PACK_STR_4('W','i','n','t'): require("expected season", (((uint32_t*)line)[1] & 0x00FFFFFF) == PACK_STR_4('e','r',' ','\0')); line += 7; semester = 3; break;
        default: return false;
    }
    require("expected year", isnumber(line[0]) && line[4] == '\t');
    result.hash = semester | (((_4atoi(line) - 2000) & 31) << 2);
    line += 5;

    require("expected section", isnumber(line[0]) && line[4] == '\t');
    size_t code = _4atoi(line);
    result.hash |= (code << 8);
    line += 5;

    require("expected course", isupper(line[0]));
    const char* end = strchr(line, '-');
    require("expected course", end != nullptr && end != line);
    result.subject = { line, (size_t)(end - line) };

    require("expected section", *end == '-');
    const char* sbegin = end + 1;
    const char* send   = strchr(sbegin, '\t');
    require("expected section", send != nullptr && send != sbegin);
    result.section = { sbegin, (size_t)(send - sbegin) };
    return true;

    #undef require
}

template <typename F>
void parseDvc (const char* filePath, const F& callback) {
    std::ifstream file { filePath };
    if (!file) {
        std::cerr << "Could not load '" << filePath << "'" << std::endl;
        exit(-1);
    } else {
        std::cout << "Loaded file '" << filePath << "'" << std::endl;
    }
    ParseResult result;
    std::string line;
    size_t lineNum = 0;
    while (getline(file, line)) {
        if (parse(filePath, ++lineNum, line.c_str(), result)) {
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




int main (int argc, const char** argv) {
    unittest_4atoi();
    std::cout << "Programmer: Seiji Emery\n"
              << "Programmer's id: M00202623\n"
              << "File: " __FILE__ "\n\n";
    parseDvc(argc, argv, [](const ParseResult& result, size_t lineNum, const std::string& line){
        report() << lineNum << ": " << result.subject << "-" << result.section;
    });
    return 0;
}
