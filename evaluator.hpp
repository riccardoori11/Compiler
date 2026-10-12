#pragma once
#include "ast.hpp"
#include "object.hpp"
#include <memory>
#include <stdexcept>
#include <vector>


using VectorStatement = std::vector<std::unique_ptr<Statement>>;

class Evaluator{

private:



public:

Evaluator() = default;

Value evalStatement(const VectorStatement& statements){

		if (statements.empty()){

				throw std::runtime_error("Empty statements");
		}

		Value result;

		for (const auto& statement: statements){

				result = eval(*statement);
		}

		return result;
}

Value eval(const Node& node){

		if (const auto* ptr = dynamic_cast<const Integer_Liter*>(&node)){
				return Value{ptr->value};
		}
		else if(const auto* ptr = dynamic_cast<const Boolean*>(&node)){
				return Value{ptr->value};
		}
		else if(const auto* ptr = dynamic_cast<const Program*>(&node)){

				return evalStatement(ptr->statements);
		}
		else if(const auto* ptr = dynamic_cast<const ExpressionStatement*>(&node)){
				return eval(*ptr->expr);
		}
		else{
				throw std::runtime_error("Unrecognised ast");
		}
}

};	
