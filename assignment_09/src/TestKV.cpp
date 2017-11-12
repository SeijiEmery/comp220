
#include <iostream>     // std::cout, std::cin
#include <string>       // std::string
#include <regex>        // std::regex, std::regex_search, std::smatch
#include <vector>       // std::pair
#include <utility>      // std::pair
#include <functional>   // std::function
#include <cstdlib>  // atoi, exit
#include "AssociativeArray.h"

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
LineWriter report () { return writeln(SET_CYAN); }
LineWriter warn   () { return writeln(SET_RED); }
LineWriter info   () { return writeln(SET_GREEN); }


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
        info() << "Creating case from " << pattern;
        info() << "Generated regex:   " << regex;
        cases.emplace_back(std::regex(regex), callback);
        return *this;
    }
    bool parse (std::string& line, std::smatch &match) {
        for (auto& case_ : cases) {
            if (std::regex_search(line, match, case_.first)) {
                case_.second(match);
                line = match.suffix().str();
                return true;
            }
        }
        return false;
    }
};

int main () {
    AssociativeArray<std::string, std::string, AADefaultStrategy> array;
    SimpleRegexParser parser;

    typedef const std::smatch& Match;
    parser
        .caseOf("quit", [&](Match match) {
            warn() << "exiting";
            exit(0);
        })
        .caseOf(".|list|print|dump", [&](Match match) {
            if (array.size() == 0) {
                report() << "[]";
            } else {
                std::cout.flush();
                std::cout << SET_CYAN "[ ";
                bool first = true;
                for (const auto& kv : array) {
                    if (first) { first = false; }
                    else { std::cout << ", "; }
                    std::cout << kv.first << " = " << kv.second;
                }
                std::cout << " ]" CLEAR_COLOR "\n";
            }
        })
        .caseOf("length", [&](Match match) {
            report() << array.size();
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
    ;
    std::string line;
    std::smatch match;
    while (1) {
        while (!getline(std::cin, line)) {}
        while (parser.parse(line, match)) {}
    }
}

