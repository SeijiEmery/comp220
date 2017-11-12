
#include <iostream>     // std::cout, std::cin
#include <sstream>      // std::stringstream
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

template <typename It>
class RSequence {
    It begin, end;
public:
    RSequence (It begin, It end) : begin(begin), end(end) {}
    operator bool () const { return begin != end; }
    RSequence& operator++ () { return ++begin, *this; }
    auto operator* () -> decltype(*begin) { return *begin; }
};
template <typename It>
auto sequence (It begin, It end) -> RSequence<It> { return { begin, end }; }

template <typename R, typename T, typename Range>
class RMap {
    Range range;
    std::function<R(T)> f;
public:
    RMap (Range range, std::function<R(T)> f) : range(range), f(f) {}
    operator bool () const { return bool(range); }
    RMap& operator++ () { return ++range, *this; }
    R     operator*  () { return f(*range); }
};

template <typename R, typename T, typename Range>
auto map (std::function<R(T)> f, Range range) -> RMap<R,T,Range> { return { range, f }; }

template <typename It, typename R, typename T>
auto map (std::function<R(T)> f, It begin, It end) -> decltype(map(f, sequence(begin, end))) {
    return map(f, sequence(begin, end));
}

template <typename R, typename T, typename Range>
auto reduce (std::function<R(R,T)> f, Range range, R first) -> R {
    for (; range; ++range) {
        first = f(first, *range);
    }
    return first;
}
template <typename T, typename Range>
auto reduce (std::function<T(T,T)> f, Range range) -> T {
    if (!range) return T();
    else {
        auto first = *range; ++range;
        for (; range; ++range) {
            first = f(first, *range);
        }
        return first;
    }
}

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

int main () {
    typedef const std::smatch& Match;
    typedef std::string Key;
    typedef std::string Value;
    typedef std::pair<Key,Value> KV;
    typedef AssociativeArray<Key, Value, AADefaultStrategy> Dict;
    
    Dict array;
    SimpleRegexParser()
        .caseOf("quit", [&](Match match) {
            warn() << "exiting";
            exit(0);
        })
        .caseOf(".|list|print|dump", [&](Match match) {
            if (array.size() == 0) {
                report() << "[]";
            } else {
                static const std::function<std::string(const KV&)> toString = [](const KV& kv) -> std::string { return kv.first + " = " + kv.second; };
                report() << "[ " << join(", ", map(toString, array.begin(), array.end())) << " ]";
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
        .parse(std::cin);
    return 0;
}

