
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

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
    // printf("%s",data);

    // Parse lines...
    char* s = data; size_t nlines = 0;
    for (; *s != '\0'; ++s) {
        while (*s && *s != '\n') ++s;
        ++nlines;
    }
    printf("%ld lines\n", nlines);

    free(data);
    return 0;
}
