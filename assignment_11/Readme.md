# Assignment 11

Implements a heap / priority queue, used in:

* an improved simulation program (from assignment 7)
* performance tests for push / pop operations
* REPL interpreter with a small DSL to thoroughly test heap (extracurricular / not part of assignment)

## Build Instructions:
    cd <some-temp-dir>
    git clone https://github.com/SeijiEmery/comp220
    cd comp220/assignment_11/
    mkdir build; cd build
    unzip ../../data/dvc-schedule.txt.zip
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make setup

### To run the simulation:
    make run

### To run benchmark tests:
    make push pop

### To run REPL interpreter / tester:
    make test

Note: the interpreter just uses a small / trivial DSL that uses regex-based pattern matching. It is still quite powerful, and its strength lies in how easy it is to extend / add new features (see PriorityQueueTest.cpp). 

Note that it's basically a concatenative language that ignores whitespace and just greedily matches everything it can. This is a useful feature: typing the following will insert N elements, then pop N elements, printing after each operation:

    ***********************
    .......................

For a full list of commands, type 'help'. To quit, type 'quit' (or ctrl+C, though this won't give you memory + timing stats).

Note also that this can be fully automated by piping in files, and can build benchmarks, and even tests built off of this. Several commandline arguments have been added to facilitate this, though you will need to build + run the executable directly. Here's an example:

    make itest
    echo "********** show sorted clear show quit" > test.txt
    cat test.txt | ./itest --help off --print off
