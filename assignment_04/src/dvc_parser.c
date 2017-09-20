
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

enum {
    SPRING = 0, SUMMER = 1, FALL = 2, WINTER = 3
};



const unsigned YEAR_ORIGIN = 2000;

typedef struct {
    unsigned semester:  2;   // 2-bit semester, following enum above
    unsigned year:     6;   // 6-bit year (0-64] value, with 0 = YEAR_ORIGIN
    unsigned section: 24;  // course id
} SectionId;

typedef struct {
    char*       subject;    // subject name (non-unique string)
    unsigned    classid;    // subject + classid => course # ("ARCHI"-130)
    SectionId   section;    // section id
} SectionInfo;

typedef struct SectionList SectionList;
struct SectionList {
    void* prev;
    void* next;
    size_t count;
    size_t nextelem;
};

SectionList* allocSectionList (size_t count, SectionList* prev, SectionList* next) {
    SectionList* node = malloc(sizeof(SectionList) + sizeof(SectionInfo) * count);
    if ((node->prev = prev)) {
        assert(!prev->next);
        prev->next = node;
    }
    if ((node->next = next)) {
        assert(!next->prev);
        next->prev = node;
    }
    node->count = count;
    node->nextelem = 0;
    memset(&node[1], 0, node->count * sizeof(SectionInfo));
    return node;
}
void freeSectionList (SectionList* head) {
    assert(head);
    while (head) {
        SectionList* next = head->next;
        free(head);
        head = next;
    }
}
SectionList* insertSection (SectionList* node, unsigned semester, unsigned year, unsigned sectionid, char* subject, unsigned classid) {
    assert(node);
    if (node->nextelem >= node->count) {
        return insertSection(
            allocSectionList(node->count, node, NULL),
            semester, year, sectionid, subject, classid);
    }
    SectionInfo* section = &((SectionInfo*)(&node[1]))[node->nextelem++];
    section->section.semester = semester;
    section->section.year     = year;
    section->section.section  = sectionid;
    section->subject = subject;
    section->classid = classid;
    return node;
}
void printSection (FILE* out, SectionInfo* section) {
    const char* semesters[4] = {
        "Spring", "Summer", "Fall", "Winter"
    };
    fprintf(out, "%s %u %u %s-%u\n", 
        semesters[section->section.semester],
        section->section.year + YEAR_ORIGIN,
        section->section.section,
        section->subject,
        section->classid
    );
}

void printSections (FILE* out, SectionList* head) {
    assert(head);
    while (head) {
        SectionInfo* list = &((SectionInfo*)(&head[1]))[0];
        SectionInfo* end  = &((SectionInfo*)(&head[1]))[head->nextelem];
        for (; list < end; ++list) {
            printSection(out, list);
        }
        head = head->next;
    }
}

SectionList* parseLineAndInsertSection (SectionList* node, char* line) {
    assert(node && line);

    // Garbage data (no parsing) just to make sure all of the above is working
    unsigned semester = FALL;
    unsigned year = 1;
    static unsigned sectionid = 1000;
    static char * subject = "FUBAR";
    unsigned classid = 101;

    return insertSection(node, semester, year, ++sectionid, subject, classid);
}



int main () {
    FILE* file = fopen("../data/dvc-schedule.txt", "r");
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, 0);

    assert(file_size < 64 * 1024 * 1024);   // impose sane limit on file size...
    printf("File has %ld bytes\n", file_size);

    // Load full file to buffer
    char* data = malloc(file_size + 1);
    {
        size_t rv = fread(data, sizeof(data[0]), file_size, file);
        if (rv != file_size) {
            fprintf(stderr, "Could not read file: %lu", rv);
            exit(rv);
        }
        data[file_size] = '\0';
    }
    SectionList* sections = allocSectionList(16 * 1024 * 1024, NULL, NULL);

    // Just load + print all data (initial test)
    char* s = data; size_t nlines = 0;
    for (; *s != '\0'; ++s) {
        sections = parseLineAndInsertSection(sections, s);
        while (*s && *s != '\n') ++s;
        ++nlines;
    }
    printSections(stdout, sections);
    printf("%ld lines\n", nlines);

    freeSectionList(sections);
    free(data);
    return 0;
}
