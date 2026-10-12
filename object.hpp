#include <variant>
#include <format>
#include <string>
#include <iostream>


/*
 * for now only represent integer
 * */

using Value = std::variant<int,bool>;
std::string Inspect(const Value& value); 

std::string_view type(const Value& value);


