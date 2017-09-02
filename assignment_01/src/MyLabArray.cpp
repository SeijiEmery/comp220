// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// Array lab exercise
//

#include <iostream>     // std::cin, std::cout, std::endl
#include <cstdlib>      // atoi()
#include <array>
#include <string>       // std::string
using namespace std;


int main () {
    std::array<int, 10> keys;   keys.fill(0);
    std::array<int, 10> values; values.fill(0);
    std::string input;
    int uniqueValuesEntered = 0;

    do {
        std::cout << "Input an index and a value [Q to quit]: ";
        std::cin >> input;
        if (input[0] == 'Q' || input[0] == 'q') {
            break;
        }
        int index = atoi(input.c_str());
        std::cin >> input;
        int value = atoi(input.c_str());

        if (index >= 0 && index < 10) {
            if (!keys[index]) {
                ++uniqueValuesEntered;
            }
            keys[index]   = 1;
            values[index] = value;
        }
    } while (1);

    std::cout << "\nYou stored this many values: " << uniqueValuesEntered << '\n';
    std::cout << "The index-value pairs are:\n";
    for (auto i = 0; i < 10; ++i) {
        if (keys[i] != 0) {
            std::cout << ' ' << i << " => " << values[i] << '\n';
        }
    }
    std::cout << '\n';

    do {
        std::cout << "Input an index for me to look up [Q to quit]: ";
        std::cin >> input;
        if (input[0] == 'Q' || input[0] == 'q') {
            break;
        }
        int index = atoi(input.c_str());
        if (index >= 0 && index < 10 && keys[index]) {
            std::cout << "Found it -- the value stored in " << keys[index] << " is " << values[index] << '\n';
        } else {
            std::cout << "I didn't find it\n";
        }
    } while (1);
}
