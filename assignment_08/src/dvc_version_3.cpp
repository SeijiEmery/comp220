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


//
// DATA STRUCTURES (generic)
//

#include <DynamicArray.h>

// Bitset data structure, used to implement a simple hashset for duplicate element removal
// (can use perfect hashing for this data set due to its unique properties).
class Bitset {
    DynamicArray<size_t> array;
    enum { BITS = 4 * sizeof(size_t) };
public:
    Bitset (size_t count) : array(count / BITS + 1) {}
    void set   (size_t i) { array[i / BITS] |=  (1 << (i % BITS)); }
    void clear (size_t i) { array[i / BITS] &= ~(1 << (i % BITS)); }
    bool get   (size_t i) { return array[i / BITS] & (1 << (i % BITS)); }
    static void unittest ();
};

// Simple unittests for bitset (assumes that our already tested DynamicArray works properly...)
void Bitset::unittest () {
    Bitset bitset (10);
    bitset.set(7);
    for (auto i = 0; i < 16; ++i) {
        assert(bitset.get(i) == (i == 7));
    }
    bitset.set(8);
    for (auto i = 0; i < 16; ++i) {
        assert(bitset.get(i) == (i == 7 || i == 8));
    }
    bitset.set(9);
    for (auto i = 0; i < 16; ++i) {
        assert(bitset.get(i) == (i == 7 || i == 8 || i == 9));
    }
    bitset.clear(9);
    bitset.clear(7);
    bitset.set(31);
    for (auto i = 0; i < 32; ++i) {
        assert(bitset.get(i) == (i == 8 || i == 31));
    }
    bitset.set(2417491);
    for (auto i = (2417491 / 8) * 8; i < (2417491 / 8 + 1) * 8; ++i) {
        assert(bitset.get(i) == (i == 2417491));
    }
}

typedef size_t hash_t;

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



//
// ACTOR FRAMEWORK
//

// enum class ActorTag : int {
//     Allocator, SubjectModel, Parser, Reader, Filterer, Counter, Sorter
// };

enum {
    kAllocator, kSubjectModel, kParser, kReader, kFilterer, kCounter, kSorter
};
typedef int ActorTag;

template <int tag, typename Actor>
struct BaseActionable {
    Actor& actor;

    BaseActionable (Actor& actor) : actor(actor) { /*actor.enter(*this);*/ }
    BaseActionable (const BaseActionable&) = delete;
    BaseActionable& operator= (const BaseActionable&) = delete;
    BaseActionable (BaseActionable&&) = default;
    BaseActionable& operator= (BaseActionable&&) = default;
    ~BaseActionable () { /*actor.exit(*this);*/ }
};

template <int tag, typename Actor, typename Allocator>
struct Actionable : public BaseActionable<tag, Actor> {
    Allocator& allocator;

    Actionable (Actor& actor, Allocator& allocator) 
        : BaseActionable<tag, Actor>(actor),
        allocator(allocator)
    {}
    Actionable (const Actionable&) = delete;
    Actionable& operator= (const Actionable&) = delete;
    Actionable (Actionable&&) = default;
    Actionable& operator= (Actionable&&) = default;
};

template <int tag, typename Impl>
struct AIS {
    template <int Tag, typename Actor, typename Allocator>
    struct Instance : public Actionable<Tag, Actor, Allocator>, public Impl::Instance {
        typedef Impl Parent;

        template <typename... Args>
        Instance (Actor& actor, Allocator& allocator, Args... args) : 
            Actionable<Tag, Actor, Allocator>(actor, allocator), 
            Impl::Instance(args...) 
        {
            static_cast<BaseActionable<Tag,Actor>*>(this)->actor.enter(*this);
        }
        Instance (const Instance&) = delete;
        Instance& operator= (const Instance&) = delete;
        Instance (Instance&&) = default;
        Instance& operator= (Instance&&) = default;
        ~Instance () {
            static_cast<BaseActionable<Tag,Actor>*>(this)->actor.exit(*this);
        }
    };

    template <typename Actor, typename Allocator, typename... Args>
    static Instance<tag, Actor, Allocator> create (Actor& actor, Allocator& allocator, Args... args) {
        return Instance<tag, Actor,Allocator>(actor, allocator, args...);
    }
};



//
// ALLOCATOR
//

struct IAllocator {
    static IAllocator* current;
    IAllocator* prev;
public:
    IAllocator () : prev(current) { current = this; }
    IAllocator (const IAllocator&) = delete;
    IAllocator& operator= (const IAllocator&) = delete;
    IAllocator& operator= (IAllocator&& other) { return std::swap(prev, other.prev), *this; }
    IAllocator (IAllocator&& other) { *this = std::move(other); }
    ~IAllocator () { current = prev; }

    virtual void* allocate (size_t size) = 0;
    virtual void deallocate (void* ptr) = 0;
};

void* operator new (size_t size) throw(std::bad_alloc) {
    void* ptr = IAllocator::current->allocate(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}
void operator delete (void* ptr) throw() {
    IAllocator::current->deallocate(ptr);
}

struct Mallocator : public IAllocator {
    void* allocate (size_t size) override { return std::malloc(size); }
    void  deallocate (void* ptr) override { std::free(ptr); }

    friend std::ostream& operator<< (std::ostream& os, const Mallocator& allocator) { return os; }
};
Mallocator mallocator;
IAllocator* IAllocator::current = static_cast<IAllocator*>(&mallocator);


struct TracingAllocator : public IAllocator {
    size_t numAllocations = 0, bytesAllocated = 0;
    size_t numDeallocations = 0, bytesDeallocated = 0;

    friend std::ostream& operator<< (std::ostream& os, const TracingAllocator& allocator) {
        return os << "Allocator:\n\t"
            << ((double)allocator.bytesAllocated * 1e-6) << " MB allocated in " << allocator.numAllocations << " allocations\n\t"
            << ((double)allocator.bytesDeallocated * 1e-6) << " MB freed in " << allocator.numDeallocations << " deallocations\n";
    }

    void* allocate (size_t size) {
        ++numAllocations;
        bytesAllocated += size;

        size_t* mem = (size_t*)(std::malloc(size + sizeof(size_t)));
        assert(mem != nullptr);
        mem[0] = size;
        return (void*)(&mem[1]);
    }
    void deallocate (void* ptr) {
        size_t* mem = &((size_t*)ptr)[-1];
        size_t size = mem[0];

        ++numDeallocations;
        bytesDeallocated += size;
        std::free(((void*)mem));
    }
};


template <typename BaseAllocator>
struct DefaultAllocator {
    template <int Tag, typename Actor>
    struct Instance : 
        public BaseActionable<Tag, Actor>, 
        public BaseAllocator 
    {    
        typedef TracingAllocator Parent;

        Instance (Actor& actor) : 
            BaseActionable<Tag, Actor>(actor), 
            BaseAllocator() 
        {
            actor.enter(*this);
        }
        Instance (const Instance&) = delete;
        Instance& operator= (const Instance&) = delete;
        Instance (Instance&&) = default;
        Instance& operator= (Instance&&) = default;
        ~Instance () {
            static_cast<BaseActionable<Tag,Actor>*>(this)->actor.exit(*this);
        }

        template <typename T>
        T* allocate (size_t count = 1) {
            // actor.onAllocation<T>(count);
            return static_cast<T*>(allocate(sizeof(T) * count));
        }
        template <typename T>
        void deallocate (T* ptr) {
            // actor.onDeallocation<T>(ptr);
            deallocate(static_cast<void*>(ptr));
        }

        template <typename T, typename... Args>
        T* create (Args... args) {
            return new(allocate<T>()) T(args...);
        }
        template <typename T>
        void destroy (T* ptr) {
            ptr->~T();
            deallocate(ptr);
        }
    };

    template <typename Actor>
    static Instance<kAllocator, Actor> create (Actor& actor) {
        return Instance<kAllocator, Actor>(actor);
    }
};

//
// SUBJECT MODEL (POD)
//

struct Subject { 
    std::string name; 
    size_t count = 0;
    size_t hashid = 0;

    Subject () = default;
    Subject (std::string name, size_t count, size_t hashid)
        : name(name), count(count), hashid(hashid) {}

    friend std::ostream& operator<< (std::ostream& os, const Subject& subject) {
        return os << "{ " << subject.name << ", " << subject.count << " (" << subject.hashid << " }";
    }
};

struct SubjectModel : public AIS<kSubjectModel, SubjectModel> {
    struct Instance {
        DynamicArray<Subject> subjects;
        size_t subjectCount = 0;
    };
};



//
// FILE READING ALGORITHMS
//
    
struct IfstreamReader : public AIS<kReader, IfstreamReader> {
    struct Instance {
        std::ifstream file;
        std::string   line_;
    public:
        Instance (const char* path) : file(path) {}
        void reset () {}
        operator bool () const { return bool(file); }
        const char* line () { return getline(file, line_), line_.c_str(); }
    };
};

struct CFileReader : public AIS<kReader, CFileReader> {
    struct Instance {
        FILE* file;
        char buf[512];
    public:
        Instance (const char* path) : file(fopen(path, "r")) {}
        ~Instance () { fclose(file); }
        void reset () {}
        operator bool () { return fgets(&buf[0], sizeof(buf) / sizeof(buf[0]), file) != nullptr; }
        const char* line () { return &buf[0]; }
    };
};

struct CFilePreBufferedReader : public AIS<kReader, CFilePreBufferedReader> {
    struct Instance {
        FILE* file;
        size_t size;
        char*  data;
        char* nextLine;
        char* nextNextLine;
    public:
        Instance (const char* path) : 
            file(fopen(path, "rb")),
            size((fseek(file, 0, SEEK_END), ftell(file))),
            data(new char[size+128])
        {
            //std::cout << "Opening file '" << path << "', size " << size << '\n';
            rewind(file);
            memset(&data[size], 0, 128);
            fread(&data[0], 1, size, file);
            reset();
        }
        ~Instance () {
            fclose(file);
            delete[] data;
        }
        void reset () { 
            //std::cout << "resetting...\n";
            nextLine = &data[0]; 
        }
        operator bool () { return (nextNextLine = strchr(nextLine + 1, '\n')) != nullptr; }
        const char* line () {
            if (nextNextLine) nextNextLine[0] = '\0';

            const char* line = nextLine;
            nextLine = &nextNextLine[1];
            return line;
        }
    };
};

// Mock version, that:
// - does no I/O
// - generates fixed # of lines
// - returns the same line repeatedly
//
// Should be used to test parser speed only, NOT duplicate checking
//
struct FakeReader : public AIS<kReader, FakeReader> {
    struct Instance {
    public:
        Instance (const char* path) {}
        void reset () {}
        operator bool () { return true; }
        const char* line () {
            return "Spring 2009\t2949\tARCHI-130\tAbbott\tTTH 8:00-10:50am ET-122A";
        }
    };
};

struct FakeRandomReader : public AIS<kReader, FakeRandomReader> {
    struct Instance {
        time_t seed;
        char buffer[512];
        const char* semesters[8] {
            "Spring", "Spring", "Spring",
            "Summer", "Summer",
            "Fall", "Fall", "Fall" 
        };
        const char* subjects[128] {
            "ADJUS", "ADS", "AET", "ANTHR", "ARABC", "ARCHI", "ART", "ARTDM", "ARTHS", 
            "ASTRO", "BCA", "BIOSC", "BUS", "BUSAC", "BUSGR", "BUSIM", "BUSMG", "BUSMK", 
            "CARDV", "CARER", "CHEM", "CHIN", "CIS", "CNT", "COLQY", "COMM", "COMSC", 
            "COMTC", "CONST", "COOP", "COUNS", "CULN", "DANCE", "DENHY", "DENTE", "DENTL", 
            "DRAMA", "ECE", "ECON", "EDUC", "ELECT", "ELTRN", "ENGIN", "ENGL", "ENGTC", 
            "ENSYS", "ENVSC", "ESL", "FAMLI", "FARSI", "FIELD", "FILM", "FRNCH", "FTVE", 
            "GEOG", "GEOL", "GRMAN", "HIST", "HORT", "HRMCU", "HRMGT", "HSCI", "HUMAN", 
            "HVACR", "IDSGN", "INTD", "ITAL", "JAPAN", "JRNAL", "KINES", "KNACT", "KNCMB", 
            "KNDAN", "KNICA", "L", "LATIN", "LRNSK", "LS", "LT", "MATEC", "MATH", "MLT", 
            "MULTM", "MUSIC", "MUSLT", "MUSPF", "MUSX", "NUTRI", "OCEAN", "PE", "PEADP", 
            "PECMB", "PEDAN", "PEIC", "PERSN", "PETHE", "PHILO", "PHYS", "PHYSC", "POLSC", 
            "PORT", "PSYCH", "RE", "RUSS", "SIGN", "SOCIO", "SOCSC", "SPAN", "SPCH", "SPEDU", 
            "SPTUT", "TAGLG", "WRKX", "ENGL", "ENGL", "ENGL", "MATH", "MATH", "MATH", "SPCH",
            "BIOL", "CHEM", "SOCIO", "PHILO", "MUSIC", "MULTM", "HUMAN", "PHYS" 
        };
    public:
        Instance (const char* filePath) : seed(time(nullptr)) { reset(); }
        void reset    () { srand(seed); }
        operator bool () { return true; }
        const char *line () {
            snprintf(&buffer[0], sizeof(buffer) / sizeof(buffer[0]),
                "%s %d\t%04d\t%s-%d\t",
                semesters[rand() % (sizeof(semesters) / sizeof(semesters[0]))],
                2000 + (rand() % 16),
                rand() % 10000,
                subjects[rand() % (sizeof(subjects) / sizeof(subjects[0]))],
                rand() % 128
            );
            //std::cout << "Generated '" << &buffer[0] << "'\n";
            return &buffer[0];
        }
    };
};

//#define parent (static_cast<typename Reader::Instance*>(this))

template <size_t COUNT, typename Reader>
struct Take : public AIS<kReader, Take<COUNT, Reader>> {
    struct Instance /*: public Reader::Instance*/ {
        typename Reader::Instance next;
        int count = (int)COUNT;
    public:
        Instance (const char* path) : next(path) {}
        void reset () { count = (int)COUNT; next.reset(); }
        operator bool () { return bool(next) && count --> 0; }
        const char* line () { return next.line(); }

        //Instance (const char* path) : Reader::Instance(path) {}
        //void reset () { count = (int)COUNT; parent->reset(); }
        //operator bool () { return bool(*parent) && count --> 0; }
    };
};


template <size_t COUNT, typename Reader>
struct Skip : public AIS<kReader, Skip<COUNT, Reader>> {
    struct Instance /*: public Reader::Instance*/ {
        typename Reader::Instance next;
    public:
        Instance (const char* path) : next(path) { skipN(); }
        void reset () { next.reset(); skipN(); }
        operator bool () { return bool(next); }
        const char* line () { return next.line(); }
        //Instance (const char* path) : Reader::Instance(path) { skipN(); }
        //void reset () { parent->reset(); skipN(); }
    private:
        void skipN () {
            for (auto i = COUNT; i --> 0 && bool(*this); ) { this->line(); }
        }
    };
};

template <typename Reader>
struct Repeat : public AIS<kReader, Repeat<Reader>> {
    struct Instance /*: public Reader::Instance */{
        typename Reader::Instance next;
    public:
        Instance (const char* path) : next(path) {}
        void reset () { next.reset(); }
        operator bool () {
            if (!bool(next)) { next.reset(); }
            return true;
        }
        const char* line () { return next.line(); }

        //Instance (const char* path) : Reader::Instance(path) {}
        //operator bool () {
        //    if (!bool(*parent)) { this->reset(); }
        //    auto result = bool(*parent);
        //    assert(result);
        //    return result;
        //}
    };
};

#undef parent





//
// PARSING ALGORITHMS
//

struct ParseResult {
    hash_t              courseHash = 0;
    const char*         line;
    Slice<const char*>  subjectStr;

    friend std::ostream& operator<< (std::ostream& os, ParseResult& result) {
        return os << "(hash: " << result.courseHash << ", subject: " << result.subjectStr.str() << ")";
    }
};

// Utility macro: assembles a 32-bit integer out of 4 characters / bytes.
// Assumes little endian, won't work on PPC / ARM (would just need to swap order).
// This is useful b/c we can replace strcmp() for very small, fixed strings
// (eg. "Fall ", "Spring "), and it's usable in a switch statement.
#define PACK_STR_4(a,b,c,d) \
    (((uint32_t)d << 24) | ((uint32_t)c << 16) | ((uint32_t)b << 8) | ((uint32_t)a))


// Heavily optimized parser implementation for the dvc-schedule file format.
// Will crash (early!) if a line is wrong and is running in debug mode. If built in release,
// will run much faster but behavior is... somewhat undefined.
struct FastParser : public AIS<kParser, FastParser> {
    struct Instance {
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
            result.courseHash = semester | (((atoi(line) - 2000) & 31) << 2);
            line += 5;

            assert(isnumber(line[0]) && line[4] == '\t');
            size_t code = atoi(line);
            result.courseHash |= (code << 8);
            line += 5;

            assert(isupper(line[0]));
            const char* end = strchr(line, '-');
            assert(end != nullptr && end != line);
            result.subjectStr = { line, (size_t)(end - line) };
            return true;
        }
    };
};

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

// Same as FastParser, but with atoi swapped out for the optimized version above 
// - turns out we're only ever parsing integers as 4 digits. Assuming strchr is optimized,
// there really isn't much of anything we can do to boost performance at this point (nor do 
// we need to - without I/O, this can parse 100k lines of text in ~5ms)
struct EvenFasterParser : public AIS<kParser, EvenFasterParser> {
    struct Instance {
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
            result.courseHash = semester | (((_4atoi(line) - 2000) & 31) << 2);
            line += 5;

            assert(isnumber(line[0]) && line[4] == '\t');
            size_t code = _4atoi(line);
            result.courseHash |= (code << 8);
            line += 5;

            assert(isupper(line[0]));
            const char* end = strchr(line, '-');
            assert(end != nullptr && end != line);
            result.subjectStr = { line, (size_t)(end - line) };
            return true;
        }
    };
};


struct NoParser : public AIS<kParser, NoParser> {
    struct Instance {
        bool parse (const char* line, ParseResult& result) { return false; }
    };
};

#undef PACK_STR_4


//
// FILTERING ALGORITHMS
//

struct HashedCourseFilterer : public AIS<kFilterer, HashedCourseFilterer> {
    struct Instance {
        Bitset hashset;
        size_t dupCount    = 0;
        size_t uniqueCount = 0;
    public:
        Instance () : hashset(0) {}
        template <typename SubjectModel>
        bool filter (const ParseResult& result, SubjectModel& model) {
            if (hashset.get(result.courseHash)) {
                ++dupCount;
                return false;
            } else {
                hashset.set(result.courseHash);
                ++uniqueCount;
                return true;
            }
        }
    };
};

// Purely for awfulness sake...
struct LinearFilter : public AIS<kFilterer, LinearFilter> {
    struct Instance {
        DynamicArray<std::string> keys;
        size_t back = 0;
    public:
        Instance () : keys(0) {}
        template <typename SubjectModel>
        bool filter (const ParseResult& result, SubjectModel& model) {
            for (auto i = back; i --> 0; ) {
                if (keys[i] == result.line) {
                    return false;
                }
            }
            keys[back++] = result.line;
            return true;
        }
    };
};

struct NoCourseFilter : public AIS<kFilterer, NoCourseFilter> {
    struct Instance {
        template <typename SubjectModel> bool filter (const ParseResult& result, SubjectModel& model) {
            return true;
        }
    };
};

//
// SUBJECT COUNTING ALGORITHMS
//

struct DefaultHash {
    static size_t hashString (const uint8_t* key, size_t len) {
        size_t hash = 0;
        for (; len --> 0; ++key) {
            hash *= 26;
            hash += (*key - 'A');
        }
        return hash;
    }
};

template <size_t HASHTABLE_SIZE = 1024, typename Hash = DefaultHash>
struct HashedSubjectCounter : public AIS<kCounter, HashedSubjectCounter<HASHTABLE_SIZE, Hash>> {
    struct Instance {
    private:
        bool emptyHash (DynamicArray<Subject>& subjects, size_t hash) const { 
            return subjects[hash].count == 0; 
        }
        bool hashEq    (DynamicArray<Subject>& subjects, size_t hash, const Slice<const char*>& key) const {
            return strncmp(subjects[hash].name.c_str(), key.start(), key.size()) == 0;
        }
    public:
        template <typename SubjectModel>
        void insert (SubjectModel& model, const ParseResult& result) {
            const auto& subject = result.subjectStr;
            size_t subjectHash = Hash::hashString((const uint8_t*)(subject.start()), subject.size());
            subjectHash %= HASHTABLE_SIZE;
            if (emptyHash(model.subjects, subjectHash)) {
                model.subjects[subjectHash] = { subject.str(), 1, subjectHash };
            } else {
                while (!hashEq(model.subjects, subjectHash, subject)) {
                    subjectHash = (subjectHash + 1) % HASHTABLE_SIZE;
                    if (emptyHash(model.subjects, subjectHash)) {
                        model.subjects[subjectHash] = { subject.str(), 1, subjectHash };
                        return;
                    }
                }
                ++model.subjects[subjectHash].count;
            }
        }
        template <typename SubjectModel>
        void finalize (SubjectModel& model) {
            model.subjectCount = 0;
            for (size_t i = 0, j = 0; i < HASHTABLE_SIZE; ++i) {
            if (model.subjects[i].count != 0) {
                if (i != model.subjectCount) {
                    std::swap(model.subjects[i], model.subjects[model.subjectCount]);
                    // ++swapCount;
                }
                ++model.subjectCount;
            }
        }
        }
    };
};

struct NoSubjectCounter : public AIS<kCounter, NoSubjectCounter> {
    struct Instance {
        template <typename SubjectModel>
        void insert (SubjectModel& model, const ParseResult& result) {}

        template <typename SubjectModel>
        void finalize (SubjectModel& model) {}
    };
};


//
// SORTING ALGORITHMS
//

struct BubbleSort : public AIS<kSorter, BubbleSort> {
    struct Instance {
        size_t swapCount = 0;

        template <typename SubjectModel>
        void sort (SubjectModel& model) {
            sort(&model.subjects[0], &model.subjects[model.subjectCount]);
        }
        void sort (Subject* front, Subject* back) {
            for (; front != back; ++front) {
                for (auto second = front; second != back; ++second) {
                    if (front->name > second->name) {
                        std::swap(*front, *second);
                        ++swapCount;
                    }
                }
            }
        }
    };
};

struct NoSort : public AIS<kSorter, NoSort> {
    struct Instance {
        template <typename SubjectModel>
        void sort (SubjectModel& model) {}
    };
};


//
// ACTORS (can run actions on ctors / dtors, or on any other custom functions (events) we define via the above Actionables)
//

struct Actor {
    template <typename Instance> void enter (const Instance& instance) {}
    template <typename Instance> void exit  (const Instance& instance) {}
};

#define ACT(tag, action) \
    template <template<int, typename...> class Instance, typename... Args> \
    struct s_##action<Instance<tag, Args...>> { static void exec (const Instance<tag, Args...>& instance)
#define ACT_all(action) \
    template <template<int, typename...> class Instance, int tag, typename... Args> \
    struct s_##action<Instance<tag, Args...>> { static void exec (const Instance<tag, Args...>& instance)
#define IMPLEMENT_ACTOR_EVENT(event) \
    template <typename T> struct s_##event { static void exec (const T&) { /*std::cout << #event "\n";*/ } }; \
    template <typename T> void event (const T& instance) { return s_##event<T>::exec(instance); }

struct DisplayToCout : public Actor {
    IMPLEMENT_ACTOR_EVENT(enter)
    IMPLEMENT_ACTOR_EVENT(exit)

    ACT(kParser, enter){ std::cout << "Parsing lines...\n"; }};
    ACT(kSorter, enter){ std::cout << "Sorting lines...\n"; }};
    ACT(kSubjectModel, exit) {
        std::cout << '\n';
        for (size_t i = 0, n = instance.subjectCount; i < n; ++i) {
            std::cout << instance.subjects[i] << '\n';
            // std::cout << instance.subjects[i].name << '\n';
        }
        std::cout << instance.subjectCount << " subjects\n";
    }};
    ACT_all(enter) {
        // typedef base_type<decltype(instance)> T;
        // std::cout << "enter: " << typeid(typename T::Parent).name() << '\n';
    }};
    ACT_all(exit) {
        // typedef typename std::remove_cv<decltype(instance)>::type T;
        // std::cout << "exit: " << typeid(typename T::Parent).name() << '\n';
    }};
    ACT(kAllocator, exit) {
        std::cout << instance << '\n';
    }};
};

struct NoDisplay : public Actor {
    IMPLEMENT_ACTOR_EVENT(exit)
    ACT(kAllocator, exit) {
        //std::cout << instance << '\n';
    }};
};

#undef ACT
#undef ACT_all
#undef IMPLEMENT_ACTOR_EVENT



//
// FULL PROGRAM IMPLEMENTATION
//

// Main dvc parsing algorithm, heavily parameterized to use any:
// – Actor (ie. what actions are taken when certain events happen)
// – Allocator (could use custom allocator to improve performance; will be used (in theory) for everything in here)
// – File I/O algorithm
// - Parser algorithm
// - Subject filtering algorithm
// - Subject counting algorithm
// - Subject sorting algorithm
//
// Note: this is heavily nested b/c RAII – we're using the actor to act (maybe), specifically on the ctors + dtors
// of the instances below.
//
// For the below code, our actors could:
//  – print subjects when subjects goes out of scope (dtor / exit)
//  - print "parsing..." when parser enters scope (ctor / begin)
//  - print "sorting..." when sorter enters scope (ctor / begin)
// OR:
//  - benchmark all read + parsing + filtering operations when those enter / exit scope
//  - benchmark all sorting operations when that enters / exits scope
// etc.
//
// As you can see (hopefully) this is extremely powerful, and the reason why the above code is so complicated:
// I needed to implement both
// – a parseLines algorithm that is orthogonally parameterized to support any combination of algorithms
//   (so I only need to write this once, not N * M * L * ... times)
// – an "actor" implementation (I'm using the term loosely) that can respond to events defined by these algorithms
//   (shared: construction & deconstruction – which actually covers most cases; specific: anything else)
//
// As such:
// - algorithms needed to be objects
// – I didn't want any overhead for the actor implementation, which meant no virtual inheritance
// - as such everything here uses compile-time polymorphism / template specialization, and is all duck-typed 
//   (ie. "invisible" interfaces; if it doesn't compile, you broke the interface somewhere)
// - algorithm classes needed tags so actor could respond to tags, not specific types
// - for the actor to switch based on tags means concrete impl of every actionable is highly parameterized
// - but since I didn't want to write all this crap out (see how parseLines is invoked in main), came up with
//   the solution of using a nested struct for the actual implementation, with an outer layer that has just
//   one static method (create) which returns an instance of the concrete implementation
// - since it turned out that I could abstract out all of this boilerplate with inheritance (and a little bit of CRTP),
//   the resulting implementation ended up being quite clean, albeit... quite complicated.
//
template <typename Actor, typename Allocator, typename Reader, typename Parser, typename Filterer, typename Counter, typename Sorter>
void parseLines (const char* filePath) {
    Actor actor;
    {
        auto allocator = Allocator::create(actor);
        {
            auto subjects = SubjectModel::create(actor, allocator);
            {
                auto counter = Counter::create(actor, allocator);
                {
                    auto parser = Parser::create(actor, allocator);
                    auto filterer = Filterer::create(actor, allocator);
                    {
                        auto reader = Reader::create(actor, allocator, filePath);
                        ParseResult result;
                        while (reader) {
                            //std::cout << reader.line() << '\n';
                            if (parser.parse(reader.line(), result) && filterer.filter(result, subjects)) {
                                counter.insert(subjects, result);
                            }
                        }
                    }
                }
                counter.finalize(subjects);
            }
            {
                auto sorter = Sorter::create(actor, allocator);
                sorter.sort(subjects);
            }
        }
    }
}

template <typename Actor, typename Allocator, typename Reader, typename Writer>
void writeToFile (const char* readPath, const char* writePath) {
    Actor actor;
    {
        auto allocator = Allocator::create(actor);
        {
            auto reader = Reader::create(actor, allocator, readPath);
            auto writer = Writer::create(actor, allocator, writePath);
            while (reader) {
                writer.writeLine(reader.line());
            }
        }
    }
}


template <typename F, typename... Args>
double benchmark (size_t iterations, F fcn, Args... args) {
    clock_t startTime = clock();
    for (size_t i = iterations; i --> 0; ) {
        fcn(args...);
    }
    clock_t endTime = clock();
    return static_cast<double>(endTime - startTime) / CLOCKS_PER_SEC * 1e3 / iterations;
}

template <typename Reader, typename Parser, typename Filterer, typename Counter, typename Sorter, size_t lines>
void runHeadlessParser (const char* filePath) {
    parseLines<
        NoDisplay,                          // action
        DefaultAllocator<Mallocator>,       // allocator
        Take<lines, Reader>,        // file loader + file actions
        Parser,                     // parsing algorithm
        Filterer,                   // filtering algorithm
        Counter,                    // counting algorithm
        Sorter                      // sorting algorithm
    >(filePath);
}
template <typename Reader, typename Parser, typename Filterer, typename Counter, typename Sorter, size_t lines, size_t limit>
void benchParserLinear (const char* filePath, size_t iterations, double expected = 0) {
    auto runtime = benchmark(iterations, &runHeadlessParser<Reader, Parser, Filterer, Counter, Sorter, lines>, filePath);
    std::cout << "parsed lines: " << std::setw(6) << lines << " time: "
        << std::setw(8) << runtime << " ms / run";
    if (expected == 0) std::cout << "  expected O(n)\n";
    else               std::cout << "  expected " << expected << '\n';
    if (lines * 2 <= limit) {
        benchParserLinear<Reader, Parser, Filterer, Counter, Sorter, lines * 2, limit>(filePath, iterations, runtime * 2);
    }
}

template <typename Reader, typename Parser, typename Filterer, typename Counter, typename Sorter, size_t lines, size_t limit>
void benchParserQuadratic (const char* filePath, size_t iterations, double expected = 0) {
    auto runtime = benchmark(iterations, &runHeadlessParser<Reader, Parser, Filterer, Counter, Sorter, lines>, filePath);
    std::cout << "parsed lines: " << std::setw(6) << lines << " time: "
        << std::setw(8) << runtime << " ms / run";
    if (expected == 0) std::cout << "  expected O(n^2)\n";
    else               std::cout << "  expected " << expected << '\n';
    if (lines * 2 <= limit) {
        benchParserQuadratic<Reader, Parser, Filterer, Counter, Sorter, lines * 2, limit>(
            filePath, iterations, runtime * 2
        );
    }
}


template <typename Reader, typename Filterer, typename Counter, typename Sorter>
void runParserBenchSuite (const char* filePath, size_t iterations) {
    std::cout << "running benchmarks with " << iterations << " iterations\n";

    std::cout << "\nNoParser:\n";
    benchParserLinear<Reader, NoParser, Filterer, Counter, Sorter, 8000,64000>(filePath, iterations);

    std::cout << "\nFastParser:\n";
    benchParserLinear<Reader, FastParser, Filterer, Counter, Sorter, 8000,64000>(filePath, iterations);

    std::cout << "\nEvenFasterParser:\n";
    benchParserLinear<Reader, EvenFasterParser, Filterer, Counter, Sorter, 8000,64000>(filePath, iterations);

    std::cout << "\nWith quadratic filtering:\n";
    benchParserQuadratic<Reader, FastParser, LinearFilter, Counter, Sorter, 200, 3200>(filePath, (iterations + 1) / 4);
}

int main (int argc, const char** argv) {
    unittest_4atoi();
    Bitset::unittest();
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

    size_t iterations = 10; // change this to increase benchmark precision (averages) at cost of runtime performance.

    std::cout << "\nPart 1: testing dvc parsing algorithms, parser + file I/O only\n";
    runParserBenchSuite<IfstreamReader, NoCourseFilter, NoSubjectCounter, NoSort>(path, iterations);

    std::cout << "\nPart 1: testing dvc parsing algorithms, parser + fake file I/O only\n";
    runParserBenchSuite<FakeReader, NoCourseFilter, HashedSubjectCounter<1024, DefaultHash>, NoSort>(path, iterations);

    std::cout << "\nPart 2: testing dvc parsing + filtering algorithms, no counting / sorting\n";
    runParserBenchSuite<IfstreamReader, HashedCourseFilterer, NoSubjectCounter, NoSort>(path, iterations);

    std::cout << "\nPart 2: testing everything\n";
    runParserBenchSuite<IfstreamReader, HashedCourseFilterer, HashedSubjectCounter<1024, DefaultHash>, BubbleSort>(path, iterations);    

    std::cout << "\nWould you like to view sample run output y / n? ";
    std::string result; std::cin >> result;
    if (result.size() && (result[0] == 'y' || result[0] == 'Y')) {
        std::cout << "\nFastParser (64k lines):\n";
        auto runtime = benchmark(1, [&](){
            parseLines<
                DisplayToCout, 
                DefaultAllocator<TracingAllocator>,
                Take<64 * 1000, IfstreamReader>,
                FastParser,
                HashedCourseFilterer,
                HashedSubjectCounter<1024, DefaultHash>,
                BubbleSort
            >(path);
        });
        std::cout << "Ran (full, 64k lines) in " << runtime << " ms\n";
    }
    return 0;
}
