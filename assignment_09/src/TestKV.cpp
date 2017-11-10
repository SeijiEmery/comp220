
#include <iostream>
#include <string>
#include <regex>
#include <cstdlib> // atoi, exit
#include "AssociativeArray.h"

int main () {
    AssociativeArray<std::string, std::string, AADefaultStrategy> array;
    std::regex assignment { "^\\s*(\\w+)\\s*\\=\\s*(\\w+)" };
    std::regex dump       { "^\\s*(\\.|print|dump)" };
    std::regex length     { "^\\s*length" };
    std::regex quit       { "^\\s*quit" };
    std::regex clear      { "^\\s*clear" };
    std::regex removal    { "^\\s*del\\s*(\\w+)" };
    std::regex variable   { "^\\s*(\\w+)" };
    std::smatch match;
    std::string line;

    while (1) {
        std::cout << "\033[0m";
        while (!getline(std::cin, line));
        std::cout << "\033[36;1m";

        while (1) {
            if (std::regex_search(line, match, assignment)) {
                auto key = match[1].str(), value = match[2].str();
                array[key] = value;
                std::cout << key << " = " << value << '\n';
            } else if (std::regex_search(line, match, dump)) {
                if (array.size() == 0) {
                    std::cout << "[]\n";
                } else {
                    std::cout.flush();
                    std::cout << "[ ";
                    bool first = true;
                    for (const auto& kv : array) {
                        if (first) { first = false; }
                        else { std::cout << ", "; }
                        std::cout << kv.first << " = " << kv.second;
                    }
                    std::cout << " ]\n";
                }
            } else if (std::regex_search(line, match, length)) {
                std::cout << array.size() << '\n';
            } else if (std::regex_search(line, match, quit)) {
                std::cout << "\033[31mexiting\033[0m\n";
                exit(0);
            } else if (std::regex_search(line, match, clear)) {
                array.clear();
            } else if (std::regex_search(line, match, removal)) {
                auto key = match[1].str();
                if (array.containsKey(key)) {
                    std::cout << "deleted " << key << '\n';
                } else {
                    std::cout << "\033[31mno key " << key << "\033[0m\n";
                }
                array.deleteKey(key);
            } else if (std::regex_search(line, match, variable)) {
                auto key = match[1].str();
                if (array.containsKey(key)) {
                    std::cout << array[key] << '\n';
                } else {
                    std::cout << "\033[31mundefined\033[0m\n";
                }
            } else break;
            line = match.suffix().str();
        }
    }
}

