Assignment 09:
    Implements an AssociativeArray container + modified dvc parser that uses said array.

Build instructions:
    cd <some-temp-dir>
    git clone https://github.com/SeijiEmery/comp220
    cd comp220/assignment_09/
    mkdir build; cd build
    unzip ../../data/dvc-schedule.txt.zip
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make test run

To just run the dvc parser:
    make run

Note: also implemented an interactive AssociativeArray test with a simple regex-based interpreter.
Uses the following commands:
    <key> = <value>    assigns array[key] = value
    <key>              shows   array[key]
    del <key>          removes key from array
    list               shows all array elements
    length             shows array length
    clear              clears array
    quit               quits program
 To run:
    make kvtest
