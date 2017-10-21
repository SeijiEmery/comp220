
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <type_traits>


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
            << allocator.bytesAllocated << " bytes allocated in " << allocator.numAllocations << " allocations\n\t"
            << allocator.bytesDeallocated << " bytes freed in " << allocator.numDeallocations << " deallocations\n";
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
        operator bool () const { return bool(file); }
        const char* line () { return getline(file, line_), line_.c_str(); }
    };
};

// Mock version, that:
// - does no I/O
// - generates fixed # of lines
// - returns the same line repeatedly
//
// Should be used to test parser speed only, NOT duplicate checking
//
template <size_t LINE_COUNT = 70000>
struct FakeReader : public AIS<kReader, FakeReader<LINE_COUNT>> {
    struct Instance {
        size_t lineCount = LINE_COUNT;
    public:
        Instance (const char* path) {}
        operator bool () const { return lineCount != 0; }
        const char* line () {
            --lineCount;
            return "Spring 2009\t2949\tARCHI-130\tAbbott\tTTH 8:00-10:50am ET-122A";
        }
    };
};


//
// PARSING ALGORITHMS
//

struct ParseResult {
    hash_t              courseHash = 0;
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


struct FastParser : public AIS<kParser, FastParser> {
    struct Instance {
        bool parse (const char* line, ParseResult& result) {
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

struct EvenFasterParser : public AIS<kParser, EvenFasterParser> {
    struct Instance {
        bool parse (const char* line, ParseResult& result) {
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
struct HashedSubjectCounter {
    struct Wrapped : public AIS<kCounter, Wrapped> {
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
        std::cout << instance << '\n';
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


int main (int argc, const char** argv) {
    unittest_4atoi();
    // Bitset::unittest();
    // std::cout << "Programmer: Seiji Emery\n"
    //           << "Programmer's id: M00202623\n"
    //           << "File: " __FILE__ "\n\n";

    // Get path from program arguments
    // const char* path = "../data/dvc-schedule.txt";
    const char* path = "/Users/semery/projects/comp220/assignment_08/data/dvc-schedule.txt";
    switch (argc) {
        case 1: break;
        case 2: path = argv[0]; break;
        default: {
            std::cerr << "usage: " << argv[0] << " [path-to-dvc-schedule.txt]" << std::endl;
            exit(-1);
        }
    }

    // Note: found out always allocates 1 << 20 bytes for stack at start of the program...

    #if 1
    parseLines<
        DisplayToCout,
        DefaultAllocator<Mallocator>,
        IfstreamReader,
        // FakeReader <76667>,
        // FakeReader <1000000000>,
        EvenFasterParser,
        HashedCourseFilterer,
        HashedSubjectCounter<1024, DefaultHash>::Wrapped,
        BubbleSort
    >(path);
    #else
    parseLines<
        NoDisplay,
        DefaultAllocator<Mallocator>,
        FakeReader<76667>,
        FastParser,
        NoCourseFilter,
        NoSubjectCounter,
        NoSort
    >(path);
    #endif
    return 0;
}
