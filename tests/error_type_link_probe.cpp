#include "object.hpp"
#include <iostream>

// Deliberately a separate translation unit from object.cpp: error 13 concerns
// the public declaration's usability by an ordinary external caller.
int main() {
    Value value{1};
    const auto name = type(value);
    std::cout << name << '\n';
    return name == "int" ? 0 : 1;
}
