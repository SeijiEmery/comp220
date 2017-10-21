#!/usr/bin/env rdmd
import std.stdio;
import std.array: split, array;
import std.algorithm: map, filter, each, count;
import std.bitmanip: BitArray;
import std.conv: parse;
import std.format: format;
import std.math: nextPow2;
import std.string: strip;
import std.typecons: tuple;


int main () {
    int[string] subject_counts;
    BitArray    hashset;
    hashset.length = 2 << 21;

    size_t count = 0;
    File("dvc-schedule.txt", "r")
        .byLine
        .filter!((a) => ++count > 1)    // ignore 1st line
        .each!((const(char)[] line) {
            try {
                auto fields  = line.strip().split('\t');
                auto term    = fields[0].split(' ');
                auto section = fields[1].parse!uint;
                auto course  = fields[2].split('-')[0];

                // Build perfect hash to filter duplicate entries
                uint hash = 0;
                switch (term[0]) {
                    case "Spring": hash = 0; break;
                    case "Summer": hash = 1; break;
                    case "Fall":   hash = 2; break;
                    case "Winter": hash = 3; break;
                    default: assert(0, format("Invalid semester string: '%s'", term[0]));
                }
                hash |= ((term[1].parse!uint - 2000) & ((1 << 5) - 1)) << 2;
                hash |= (section & ((1 << 14) - 1)) << 7;

                if (hash >= hashset.length) {
                    writefln("resizing %s => %s", hashset.length, hash.nextPow2);
                    hashset.length = hash.nextPow2;
                }
                if (!hashset[hash]) {
                    hashset[hash] = true;
                    if (course in subject_counts) ++subject_counts[course];
                    else subject_counts[cast(const(char)[])(course.dup)] = 1;
                }
            } catch (std.conv.ConvException e) {
                writefln("error parsing line '%s'", line);
            }
        })
    ;
        //.uniq
        //.map!((a) => a.split('\t')[2].split('-')[0])
        //.each!((const(char)[] subject) {
        //    if (subject in subject_counts) ++subject_counts[subject];
        //    else subject_counts[cast(const(char)[])(subject.dup)] = 1;
        //})
    //;
    writefln("total: %s", subject_counts
        .byKeyValue
        .map!((a) => tuple(a.key, a.value))
        .array
        .sort
        .map!((a) => (writefln("%s: %s", a[0], a[1]), a))
        .count
    );
    return 0;    
}
