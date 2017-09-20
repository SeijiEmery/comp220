
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




    fclose(file);
    return 0;
}
