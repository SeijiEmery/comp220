Assignment 10:
    Implements an open-addressed hashtable, with several tests + applications

Build instructions (setup):
    cd <some-temp-dir>
    git clone https://github.com/SeijiEmery/comp220
    cd comp220/assignment_10/
    mkdir build; cd build
    unzip ../../data/dvc-schedule.txt.zip
    cmake .. -DCMAKE_BUILD_TYPE=Release

Run unittests:
    make test

Run DVC (big data) parser / impl (for assignment):
    make run

Run interactive hashtable test program (extracurricular, not part of assignment):
    make kvtest

This last program is basically a REPL w/ a very simple regex-based interpreter, and several operations to modify and inspect my hashtable implementation in real time; it's also an improved version of the program I wrote for assignment 09.

Commands:
    <key> = <value>    assigns key => value
    <key>              shows   value at key
    del <key>          removes key

    list               shows a list of all keys / values
    show               shows the internal memory layout of the hashtable (w/ set / empty elements)
    info               shows hashtable info / stats and a bitmask of set values (1 set / 0 unset)

    size               get size of hashtable
    capacity           get capacity of hashtable
    lf                 get load factor of hashtable
    lf <[1,99]>        set load factor of hashtable (as integer, 1-99)

    resize <size>      resizes hashtable (sets capacity)
    clear              removes all elements from hashtable

    fill <min> <max>   inserts elements from min to max (interpreted as integers using atoi()).
                       also, displays hashtable info after each insertion (same as 'info')

    displayfill <min> <max>    same as fill, but also clears the screen after each insertion

    quit               exits program (or ctrl+C)

You can do some interesting things with this, eg. investigate effect of loadfactors by running

    lf <insert load factor here>
    displayfill 1 10000             (or any suitably high number)

Or play around with resizing, changing the loadfactor (both rehash elements), checking
hash collisions w/ very high load factors + manual inserts / deletes, etc.
