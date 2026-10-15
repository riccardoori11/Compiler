#include <variant>
#include <format>
#include <string>
#include <iostream>


/*
 * std::monostate represents a Null state
 * */


using Value = std::variant<std::monostate,int,float,double,bool>;
std::string Inspect(const Value& value); 

std::string_view type(const auto& value);


