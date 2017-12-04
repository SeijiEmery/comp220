// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// MySortableArray.cpp
// Main program for a sortable dynamic array implementation (assignment 12).
//
// Remote source: 
// https://github.com/SeijiEmery/comp220/tree/master/assignment_12/src/MySortableArray.cpp
//

#include <iostream>
#include <string>
using namespace std;

#include <cstdlib>

#include "SortableArray.h"

int main () {
    std::cout << "Programmer:       Seiji Emery\n";
    std::cout << "Programmer's ID:  M00202623\n";
    std::cout << "File:             " << __FILE__ << '\n' << std::endl;

    // Run main program
    SortableArray<bool>    keys;   
    SortableArray<double>  values; 
    std::string input;

    do {
        std::cout << "Input an index and a value [Q to quit]: ";
        std::cin >> input;
        if (input[0] == 'Q' || input[0] == 'q') {
            break;
        }
        int index = atoi(input.c_str());
        std::cin >> input;
        double value = atof(input.c_str());

        keys[index]   = true;
        values[index] = value;
    } while (1);

    keys[-1] = false;   // clear out of bounds values...

    // Calculate unique user inputs
    int uniqueValuesEntered = 0;
    for (auto i = 0; i < keys.capacity(); ++i) {
        if (keys[i]) {
            ++uniqueValuesEntered;
        }
    }

    // Display all inputs
    std::cout << "\nYou stored this many values: " << uniqueValuesEntered << '\n';
    std::cout << "The index-value pairs are:\n";
    for (auto i = 0; i < keys.capacity(); ++i) {
        if (keys[i]) {
            std::cout << ' ' << i << " => " << values[i] << '\n';
        }
    }
    std::cout << '\n';

    // Interactively querry for inputs
    do {
        std::cout << "Input an index for me to look up [Q to quit]: ";
        std::cin >> input;
        if (input[0] == 'Q' || input[0] == 'q') {
            break;
        }
        int index = atoi(input.c_str());
        if (keys[index]) {
            std::cout << "Found it -- the value stored in " << index << " is " << values[index] << '\n';
        } else {
            std::cout << "Sorry, but there is no value stored at " << index << '\n';
        }
    } while (1);
    return 0;;
}
