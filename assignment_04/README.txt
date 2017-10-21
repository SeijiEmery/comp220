DVC Assignments 4 + 5:
Implements a program that loads a 70k line plaintext file with class schedules (dvc-schedule.txt),
skips duplicates (records may be duplicated), counts the number of sections for each class subject,
and prints these, sorted alphabetically.

Build instructions:
    cd <some-temp-dir>
    git clone https://github.com/SeijiEmery/comp220
    cd comp220/assignment_04/
    mkdir build; cd build
    unzip ../../data/dvc-schedule.txt.zip
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make dvc4; time ./dvc4
    make dvc5; time ./dvc5
