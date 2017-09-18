
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
using namespace std;

#include "../../assignment_03/src/Array.h"

class Bitset {
    Array<size_t> array;
    enum { BITS = 8 * sizeof(size_t) };
public:
    Bitset (size_t count) : array(count / BITS + 1) {}
    void set   (size_t i) { array[i / BITS] |=  (1 << (i % BITS)); }
    void clear (size_t i) { array[i / BITS] &= ~(1 << (i % BITS)); }
    bool get   (size_t i) { return array[i / BITS] & (1 << (i % BITS)); }
};

struct Field { 
    std::string name; 
    size_t count = 0; 

    Field () = default;
    Field (decltype(name) name, decltype(count) count) 
        : name(name), count(count) {}
};

#define PACK_STR_4(a,b,c,d) \
    (((uint32_t)d << 24) | ((uint32_t)c << 16) | ((uint32_t)b << 8) | ((uint32_t)a))
    // (((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | ((uint32_t)d))

int main () {
    ifstream file { "../data/dvc-schedule.txt" };
    string line;

    std::cout << "Parsing lines...";

    // Array<string> lines (1); size_t count = 0;

    size_t dupcount = 0, uniquecount = 0;

    Bitset hashset (1);
    Array<Field> fields; size_t numfields = 0; size_t linecount = 0;
    while (getline(file, line)) {
        char* s = const_cast<char*>(line.c_str());
        // std::cout << s << '\n';

        size_t semester = 0;
        switch (((uint32_t*)s)[0]) {
            case PACK_STR_4('S','p','r','i'): assert((((uint32_t*)s)[1] & 0x00FFFFFF) == PACK_STR_4('n','g',' ','\0')); s += 7; semester = 0; break;
            case PACK_STR_4('S','u','m','m'): assert((((uint32_t*)s)[1] & 0x00FFFFFF) == PACK_STR_4('e','r',' ','\0')); s += 7; semester = 1; break;
            case PACK_STR_4('F','a','l','l'): assert(s[4] == ' ');                                                      s += 5; semester = 2; break;
            case PACK_STR_4('W','i','n','t'): assert((((uint32_t*)s)[1] & 0x00FFFFFF) == PACK_STR_4('e','r',' ','\0')); s += 7; semester = 3; break;
            default: if (linecount == 0) { continue; } else { assert(0); }
        }
        // std::cout << "Season = " << hash;
        assert(isnumber(s[0]) && s[4] == '\t');
        size_t hash = semester | (((atoi(s) - 2001) & 31) << 2);
        // std::cout << "YEAR_CODE: 0x" << std::hex << hash;
        // std::cout << ", year = " << ((atoi(s) - 2001) & 32);
        s += 5;

        assert(isnumber(s[0]) && s[4] == '\t');
        size_t code = atoi(s);
        hash |= (code << 7);
        s += 5;
        // std::cout << " CLASS_ID: 0x" << std::hex << code << " HASH: 0x" << std::hex << hash << ' ';
        // std::cout << "hash = " << hash << ", line = " << line << '\n';

        if (hashset.get(hash)) {
            // std::cout << "DUPLICATE: " << line << '\n';
            ++dupcount;
            continue;
        } else {
            ++uniquecount;
        }
        // std::cout << "INSERTING: " << line << '\n';
        hashset.set(hash);

        #ifdef GROUP_SUBJECTS
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
        for (auto k = 0; k < numfields; ++k) {
            if (strcmp(fields[k].name.c_str(), s) == 0) {
                ++fields[k].count;
                goto end;
            }
        }
        fields[numfields++] = Field({ s }, 1);
        end:
        // Update progress thingy...
        if (((++linecount) % 2000) == 0) {
            std::cout << '.'; std::cout.flush();
        }
    }
    std::cout << '\n';

    std::cout << "parsed " << uniquecount << " fields, removed " << dupcount << " duplicates\n";

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
}







