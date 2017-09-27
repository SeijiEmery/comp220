
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

#include "../../assignment_03/src/DynamicArray.h"


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

struct Field { 
    std::string name; 
    size_t count = 0; 

    Field () = default;
    Field (decltype(name) name, decltype(count) count) 
        : name(name), count(count) {}
};

#define PACK_STR_4(a,b,c,d) \
    (((uint32_t)d << 24) | ((uint32_t)c << 16) | ((uint32_t)b << 8) | ((uint32_t)a))

int main () {
    ifstream file { "../data/dvc-schedule.txt" };
    string line;

    ofstream duplicate_log { "duplicates.cpp.txt" };

    std::cout << "Parsing lines...";

    size_t dupcount = 0, uniquecount = 0;

    Bitset hashset (1);
    DynamicArray<Field> fields; size_t numfields = 0; size_t linecount = 0;
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
        for (auto k = 0; k < numfields; ++k) {
            if (strcmp(fields[k].name.c_str(), s) == 0) {
                ++fields[k].count;
                goto end;
            }
        }
        fields[numfields++] = Field({ s }, 1);
        end:

        // Update progress counter
        if (((++linecount) % 1000) == 0) {
            // std::cout << '.'; std::cout.flush();
        }
    }
    std::cout << '\n';


    // Sort elements (naive / bubble sort)
    std::cout << "Sorting sections...";
    for (auto i = 1; i < numfields; ++i) {
        for (auto j = i; j < numfields; ++j) {
            if (fields[i].name > fields[j].name) {
                swap(fields[i], fields[j]);
            }
        }
        // Update progress thingy...
        if ((i % (numfields / 25)) == 0) { 
            // std::cout << '.'; std::cout.flush(); 
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
    std::cout << '\n';
    std::cout << "total: " << totalSections << " sections\n";
    std::cout << "parsed " << uniquecount << " fields, removed " << dupcount << " duplicates\n";
    std::cout << "lines: " << linecount << "\n";
}







