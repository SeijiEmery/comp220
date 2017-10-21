
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>



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
    T start, end;

    Slice () : Slice(nullptr, 0) {}
    Slice (T start, T end) : start(start), end(end) {}
    Slice (T start, size_t size) : start(start), end (&start[size]) {}
    Slice (const Slice&) = default;
    Slice& operator= (const Slice&) = default;
    operator bool () const { return start != nullptr; }
    size_t size () const { assert(start >= end); return start - end; }
    std::string str () const { return std::string(start, size()); }
};



//
// ACTOR FRAMEWORK
//

enum class ActorTag {
    Allocator, SubjectModel, Parser, Reader, Filterer, Counter, Sorter
};

template <ActorTag tag, typename Actor>
struct BaseActionable {
    Actor& actor;

    BaseActionable  (Actor& actor) : actor(actor) { actor.enter(*this); }
    ~BaseActionable () { actor.exit(*this); }
};

template <ActorTag tag, typename Actor, typename Allocator>
struct Actionable : public BaseActionable<tag, Actor> {
    Allocator& allocator;

    Actionable (Actor& actor, Allocator& allocator) 
        : BaseActionable<tag, Allocator>(actor),
        allocator(allocator)
    {}
};

template <ActorTag tag, typename Impl>
struct AIS {
    template <typename Actor, typename Allocator>
    static auto create (Actor& actor, Allocator& allocator) -> Instance<Actor, Allocator> {
        return Instance(actor, allocator);
    }
    template <typename Actor, typename Allocator>
    struct Instance : public Actionable<tag, Actor, Allocator>, public T::Instance {
        template <typename... Args>
        Instance (Actor& actor, Allocator& allocator, Args args...) : 
            Actionable<tag, Actor, Allocator>(), 
            T::Instance(args...) 
        {}
        ~Instance () {}
    }
};



//
// ALLOCATOR
//

struct BaseAllocator {
private:
    static BaseAllocator    rootAllocator;
    static BaseAllocator*   currentAllocator;
    BaseAllocator*          prevAllocator;
public:
    size_t numAllocations = 0, bytesAllocated = 0;
    size_t numDeallocations = 0, bytesDeallocated = 0;

    BaseAllocator () : prevAllocator(currentAllocator) {
        currentAllocator = this;
    }
    ~BaseAllocator () {
        currentAllocator = prevAllocator;
    }

    void* allocate (size_t size) {
        ++numAllocations;
        bytesAllocated += size;

        size_t* mem = reinterpret_cast<size_t*>(malloc(size));
        mem[0] = size;
        return reinterpret_cast<void*>(mem[1]); 
    }
    void deallocate (void* ptr) {
        size_t* mem = &reinterpret_cast<size_t*>(ptr)[-1];
        size_t size = mem[0];

        ++numDeallocations;
        bytesDeallocated += size;
        free(reinterpret_cast<void*>(ptr));
    }

    friend void* operator new (size_t size) throw(std::bad_alloc) {
        void* ptr = currentAllocator->allocate(size);
        if (!ptr) throw std::bad_alloc();
        return ptr;
    }
    friend void operator delete (void* ptr) throw() {
        return currentAllocator->deallocate(ptr);
    }
};
BaseAllocator BaseAllocator::rootAllocator;
BaseAllcator* BaseAllocator::currentAllocator = &BaseAllocator::rootAllocator;


struct DefaultAllocator {
    template <typename Actor>
    struct Instance : public BaseActionable<ActorTag::Allocator, Actor>, public BaseAllocator {
        DefaultAllocator (Actor& actor)
            : BaseActionable<ActorTag::Allocator, Actor>(actor), BaseAllocator() 
        {}
        ~DefaultAllocator () {}

        template <typename T>
        T* allocate (size_t count = 1) {
            actor.onAllocation<T>(count);
            return static_cast<T*>(allocate(sizeof(T) * count));
        }
        template <typename T>
        void deallocate (T* ptr) {
            actor.onDeallocation<T>(ptr);
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
};

struct SubjectModel : public AIS<ActorTag::SubjectModel, SubjectModel> {
    struct Instance {
        DynamicArray<Subject> subjects;
        size_t subjectCount = 0;
    };
};



//
// FILE READING ALGORITHMS
//

struct IfstreamReader : public AIS<ActorTag::Reader, IfstreamReader> {
    struct Instance {
        std::ifstream file;
        std::string   line;
    public:
        Instance (const char* path) : file(path) {}
        operator bool () const { return file; }
        const char* line () { return getline(file, line), line.c_str(); }
    };
};



//
// PARSING ALGORITHMS
//

struct ParseResult {
    hash_t              courseHash = 0;
    Slice<const char*>  subjectStr;
};

struct FastParser : public AIS<ActorTag::Parser, IfstreamReader> {
    struct Instance {
        bool parse (const char* line, ParseResult& result) {
            size_t semester = 0;
            switch (((uint32_t*)line)[0]) {
                case PACK_STR_4('S','p','r','i'): assert((((uint32_t*)line)[1] & 0x00FFFFFF) == PACK_STR_4('n','g',' ','\0')); line += 7; semester = 0; break;
                case PACK_STR_4('S','u','m','m'): assert((((uint32_t*)line)[1] & 0x00FFFFFF) == PACK_STR_4('e','r',' ','\0')); line += 7; semester = 1; break;
                case PACK_STR_4('F','a','l','l'): assert(s[4] == ' ');                                                         line += 5; semester = 2; break;
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
            result.subjectStr = { line, end };
            return true;
        }
    };
};



//
// FILTERING ALGORITHMS
//

struct HashedCourseFilterer : public AIS<ActorTag::Filterer, HashedCourseFilterer> {
    struct Instance {
        Bitset hashset;
        size_t dupCount    = 0;
        size_t uniqueCount = 0;
    public:
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
    struct Instance : public AIS<ActorTag::Counter, Instance> {
        struct Instance {
        private:
            bool emptyHash (DynamicArray<Subject>& subjects, size_t hash) const { 
                return subjects[hash].count == 0; 
            }
            bool hashEq    (DynamicArray<Subject>& subjects, size_t hash, const Slice<const char*>& key) const {
                return strcmpl(subjects[hash].name, key.start, key.size()) == 0;
            }
        public:
            void insert (DynamicArray<Subject>& subjects, const ParseResult& result) {
                const auto& subject = result.subjectStr;
                size_t subjectHash = Hash::hashString(subject.start, subject.size());
                subjectHash %= HASHTABLE_SIZE;
                if (emptyHash(subjects, subjectHash)) {
                    subjects[subjectHash] = { subject.str(), 1, subjectHash };
                } else {
                    while (!hashEq(subjects, subjectHash, subject)) {
                        subjectHash = (subjectHash + 1) % HASHTABLE_SIZE;
                        if (emptyHash(subjects, subjectHash)) {
                            subjects[subjectHash] = { subject.str(), 1, subjectHash };
                            return;
                        }
                    }
                    ++subjects[subjectHash].count;
                }
            }
        };
    };
};



//
// SORTING ALGORITHMS
//

struct BubbleSort : public AIS<ActorTag::Sorter, BubbleSort> {
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
                        std::swap(front, second);
                        ++swapCount;
                    }
                }
            }
        }
    };
};



//
// ACTORS (can run actions on ctors / dtors, or on any other custom functions (events) we define via the above Actionables)
//

#define ACT(tag, action) \
    template <typename Instance<ActorTag::tag>> void action (const Instance& instance)
#define ACT_args(tag, action, args...) \
    template <typename Instance<ActorTag::tag>> void action (const Instance& instance, args)
#define ACT_allocation(action) \
    template <typename T, typename Instance<Tag::Allocator>> void action (const Instance& allocator, size_t count)

struct Actor {
    template <typename ActorTag tag, typename Instance<tag>> void enter (const Instance& instance) {}
    template <typename ActorTag tag, typename Instance<tag>> void exit  (const Instance& instance) {}
    ACT_allocation(onAllocation)   {}
    ACT_allocation(onDeallocation) {}
};


struct DisplayToCout : public Actor {
    ACT(Parser, enter) { std::cout << "Parsing lines...\n"; }
    ACT(Parser, exit)  { std::cout << "Finished parsing\n"; }
    ACT(SubjectModel, exit) {
        std::cout << instance.subjects.size() << " subjects:\n";
        for (const auto& subject : instance.subjects) {
            std::cout << subject.name << '\n';
        }
    }

    ACT_allocation(onAllocation) {
        std::cout << "Allocated ";
        if (count) std::cout << count << " ";
        std::cout << typeid(T)::name() << " object(s) (" << (count * sizeof(T)) << " bytes)\n";
    }
    ACT_allocation(onDeallocation) {
        std::cout << "Freed "
        if (count) std::cout << count << " ";
        std::cout << typeid(T)::name() << " object(s) (" << (count * sizeof(T)) << " bytes)\n";
    }
    ACT(Allocator, exit) {
        std::cout << "allocated " << instance.bytesAllocated << " bytes in " << instance.numAllocations << " allocations\n";
        std::cout << "freed     " << instance.bytesDeallocated << " bytes in " << instance.numDeallocations << " deallocations\n";
    }
};
#undef ACT
#undef ACT_args
#undef ACT_allocation



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
            auto subjects = SubjectsModel::create(actor, allocator);
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
            }
            {
                auto sorter = Sorter::create(actor, allocator);
                sorter.sort(subjects);
            }
        }
    }
}

int main (int argc, const char** argv) {
    Bitset::unittest();
    std::cout << "Programmer: Seiji Emery\n"
              << "Programmer's id: M00202623\n"
              << "File: " __FILE__ "\n\n";

    // Get path from program arguments
    const char* path = nullptr;
    switch (argc) {
        case 1: path = "../data/dvc-schedule.txt"; break;
        case 2: path = argv[0]; break;
        default: {
            std::cerr << "usage: " << argv[0] << " [path-to-dvc-schedule.txt]" << std::endl;
            exit(-1);
        }
    }

    parseLines<
        DisplayToCout,
        DefaultAllocator,
        IfstreamReader, 
        FastParser,
        HashedCourseFilterer,
        HashedSubjectCounter<1024, DefaultHash>::Instance,
        BubbleSort,
    >();
    return 0;
}
