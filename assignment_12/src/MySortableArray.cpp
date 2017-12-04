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
#include <utility>
using namespace std;

#include <cstdlib>

#include "SortableArray.h"

int main () {
    std::cout << "Programmer:       Seiji Emery\n";
    std::cout << "Programmer's ID:  M00202623\n";
    std::cout << "File:             " << __FILE__ << '\n' << std::endl;

    // Run main program
    SortableArray<std::pair<bool, double>> values;
    std::string input;

    std::cout << "Enter pairs: (Q to quit)\n";
    do {
        std::cin >> input;
        if (input[0] == 'Q' || input[0] == 'q') {
            break;
        }
        int index = atoi(input.c_str());
        std::cin >> input;
        double value = atof(input.c_str());
        values[index] = { true, value };
    } while (1);

    values[-1] = { false, 0 };   // clear out of bounds values...

    // Calculate unique user inputs
    int uniqueValuesEntered = 0;
    for (auto i = 0; i < values.capacity(); ++i) {
        if (values[i].first) {
            ++uniqueValuesEntered;
        }
    }

    // Display all inputs
    for (auto i = 0; i < values.capacity(); ++i) {
        if (values[i].first) {
            std::cout << ' ' << i << " => " << values[i].second << " ";
        }
    }
    std::cout << '\n';

    std::cout << "Enter number of values to sort: ";
    std::cin >> input;
    size_t count = atoi(input.c_str());
    values.sort(count);

    // Display all inputs
    for (auto i = 0; i < values.capacity(); ++i) {
        if (values[i].first) {
            std::cout << ' ' << i << " => " << values[i].second << " ";
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
        if (values[index].first) {
            std::cout << "Found it -- the value stored in " << index << " is " << values[index].second << '\n';
        } else {
            std::cout << "Sorry, but there is no value stored at " << index << '\n';
        }
    } while (1);
    return 0;;
}
