#pragma once

#include "token.hpp"
#include <optional>
#include <unordered_map>
#include <variant>
#include <format>
#include <string>
#include <iostream>


/*
 * std::monostate represents a Null state
 * */




using Value = std::variant<std::monostate,int,float,double,bool>;
struct ExecutionResult{

		Value value{std::monostate{}};
		bool is_return{false};

};

struct Binding{

		TokenType type;
		std::optional<Value> value;

};

std::string Inspect(const Value& value); 

std::string_view type(const auto& value);


