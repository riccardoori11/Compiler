#pragma once

#include "ast.hpp"
#include "token.hpp"
#include <memory>
#include <optional>
#include <unordered_map>
#include <variant>
#include <format>
#include <string>
#include <string_view>
#include <iostream>
#include <vector>

/*
 * std::monostate represents a Null state
 * */


struct EnvironmentFrame;

struct Parameter{
		std::string name;
		TokenType parameter_type;
};

struct FunctionObj{
		std::vector<Parameter> parameters;
		TokenType function_type{TokenType::ILLEGAL};
		std::shared_ptr<BlockStatements> block;
		std::weak_ptr<EnvironmentFrame> env;

};

using Value = std::variant<std::monostate,int,float,double,bool,std::shared_ptr<FunctionObj>>;
struct ExecutionResult{

		Value value{std::monostate{}};
		bool is_return{false};

};

struct Binding{

		TokenType type;
		std::optional<Value> value;

};



std::string Inspect(const Value& value); 

std::string_view type(const Value& value);


