
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
using namespace std;

#include "../../assignment_03/src/Array.h"

struct Field { 
    std::string name; 
    size_t count = 0; 

    Field () = default;
    Field (decltype(name) name, decltype(count) count) 
        : name(name), count(count) {}
};


int main () {
    ifstream file { "../data/dvc-schedule.txt" };
    string line;

    std::cout << "Parsing lines...";

    // Array<string> lines (1); size_t count = 0;
    Array<Field> fields; size_t numfields = 0; size_t linecount = 0;
    while (getline(file, line)) {
        size_t field = 0;
        size_t i = 0; size_t n = line.size();
        for (; i < n; ++i) {
            if (line[i] == '\t' && ++field == 2) {
                auto start = i+1;

                // Group by individual class code or by subject group, switched with -D GROUP_SUBJECTS (faster)
                #ifdef GROUP_SUBJECTS
                    while (++i < n && line[i] != '-') {}
                #else
                    while (++i < n && line[i] != '\t') {}
                #endif

                auto key = line.substr(start, i - start);
                for (auto k = 0; k < numfields; ++k) {
                    if (fields[k].name == key) {
                        ++fields[k].count;
                        goto end;
                    }
                }
                // if ((numfields % 50) == 25) { std::cout << '.'; std::cout.flush(); }
                // std::cout << "Adding field " << key << "(line: " << line << ")" << '\n';
                fields[numfields++] = Field(key, 1);
                goto end;
            }
        }
        end:
        // Update progress thingy...
        if (((++linecount) % 2000) == 0) {
            std::cout << '.'; std::cout.flush();
        }
    }
    std::cout << '\n';

    // Sort elements (naive)
    std::cout << "Sorting sections...";
    for (auto i = 1; i < numfields; ++i) {
        for (auto j = i; j < numfields; ++j) {
            if (fields[i].name > fields[j].name) {
                swap(fields[i], fields[j]);
            }
        }
        // Update progress thingy...
        if ((i % (numfields / 25)) == 0) { 
            std::cout << '.'; std::cout.flush(); 
        }
    }
    std::cout << '\n';    

    // Display elements
    size_t totalSections = 0;
    for (auto i = 1; i < numfields; ++i) {
        if (fields[i].count) {
            std::cout << fields[i].name << ", " << fields[i].count << " sections\n";
        }
        totalSections += fields[i].count;
    }
    std::cout << "total: " << totalSections << " sections\n";
    std::cout << "lines: " << linecount << "\n";

    // while (getline(file, lines[count])) {
    //     lines[++count] = {};
    // }
    // std::cout << count << " lines\n";
    // Array<string> subjects;
    // for (auto k = 0; k < count; ++k) {
    //     auto& line = lines[k];
    //     size_t field = 0;
    //     for (size_t i = 0, n = line.size(); i < n; ++i) {
    //         if (line[i] == '\t') {
    //             if (++field == 2) {
    //                 auto j = i;
    //                 while (++i < n && line[i] != '-') {}
    //                 std::cout << line.substr(j, i) << '\n';
    //             }
    //         }
    //     }
    // }
}







