// Programmer: Seiji Emery
// Programmer ID: M00202623
//
// MyRPNCalculator.cpp
//

#include <iostream>
#include <string>
#include <regex>
using namespace std;

#include <cstdlib>
#include <cmath>
#include "Stack.h"

template <typename T>
T popBack (LinkedListStack<T>& stack, T default_ = T()) {
    T back = stack.empty() ? default_ : stack.peek();
    stack.pop();
    return back;
}

int main () {
    std::cout << "Programmer:       Seiji Emery\n";
    std::cout << "Programmer's ID:  M00202623\n";
    std::cout << "File:             " << __FILE__ << '\n' << std::endl;

    LinkedListStack<double> values;
    std::string line, input;
    bool        running = true;

    std::regex  expr { "\\s*([0-9\\.]+|[\\+\\-\\*\\^\\/qQ]|help|drop|dup|disp|swap|clear|top|pi|sin|cos|tan|asin|acos|atan|sqrt|abs)\\s*" };
    std::smatch match;
    double a, b;

    while (running) {
        std::cin >> line;
        int opcount = 0;    // # operations since last disp / eol
        for (; std::regex_search(line, match, expr); line = match.suffix().str()) {
            auto token = match[1].str();
            // std::cout << "Token '" << token << "'\n";
            #define CASE_BIN_OP(chr,op) \
                case chr: values.push(popBack(values) op popBack(values)); break;

            switch (token[0]) {
                case '+': { auto rhs = popBack(values), lhs = popBack(values, 0.0); values.push(lhs + rhs); } break;
                case '-': { auto rhs = popBack(values), lhs = popBack(values, 0.0); values.push(lhs - rhs); } break;
                case '*': { auto rhs = popBack(values), lhs = popBack(values, 1.0); values.push(lhs * rhs); } break;
                case '/': { auto rhs = popBack(values), lhs = popBack(values, 1.0); values.push(lhs / rhs); } break;
                case '^': { auto rhs = popBack(values), lhs = popBack(values, 1.0); values.push(pow(lhs, rhs)); } break;
                case 'd': 
                    if (token == "drop") { values.pop(); } 
                    else if (token == "dup" && !values.empty()) { values.push(values.peek()); }
                    else if (token == "disp") {
                    display:
                        // std::cout << opcount << "STACK: ";
                        if (values.empty()) {
                            std::cout << "empty\n";
                        } else {
                            auto copy = values;
                            while (!copy.empty()) {
                                std::cout << copy.peek() << " ";
                                copy.pop();
                            }
                            std::cout << "\n";
                        }   
                    }
                    break;
                case 't': 
                    if (token == "top") {
                        if (values.empty()) std::cout << "empty\n";
                        else std::cout << values.peek() << '\n';
                    } else if (token == "tan") {
                        values.push(tan(popBack(values)));
                    } 
                    break;
                case 's': 
                    if (token == "swap" && values.size() >= 2) { 
                        auto top = popBack(values), btm = popBack(values); 
                        values.push(top); values.push(btm);
                    } else if (token == "sin" && values.size() >= 1) {
                        values.push(sin(popBack(values)));
                    } 
                break;
                case 'c': 
                    if (token == "clear") {
                        values.clear();
                    } else if (token == "cos" && values.size() >= 1) {
                        values.push(cos(popBack(values)));    
                    }
                break;
                case 'a':
                    if (token == "asin") {
                        values.push(asin(popBack(values)));
                    } else if (token == "acos") {
                        values.push(acos(popBack(values)));
                    } else if (token == "atan") {
                        values.push(atan(popBack(values)));
                    }
                break;
                case 'p':
                    if (token == "pi") {
                        // clever way to calculate pi – from https://stackoverflow.com/a/1727886
                        values.push(atan(1) * 4);
                    }
                break;
                case 'h': if (token == "help") {
                    std::cout << "RPL Calculator. Enter number, or any of the following operations:\n"
                        << "\t+, -, *, /, ^, sin, cos, tan, acos, asin, atan, sqrt, abs, drop, dup, disp, swap, quit, clear, top, help\n";
                } break;
                case '.': 
                    if (token == ".") {
                        goto display;
                    } else goto number;
                    break;
                case 'q': case 'Q': running = false; goto quit;
                default: number: values.push(atof(token.c_str()));
            }
            ++opcount;
        }
    quit:
        continue;
    }
    return 0;
}
