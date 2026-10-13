#pragma once
#include "ast.hpp"
#include "object.hpp"
#include "token.hpp"
#include <limits>
#include <memory>
#include <stdexcept>
#include <variant>
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

Value evalPrefixExpression(const Token& op, const Value& right){

		switch (op.tokentype){

				case TokenType::MINUS:
				 {if (!std::holds_alternative<int>(right)){

								throw std::runtime_error("Invalid right expression for MINUS");
						}

						int val = std::get<int>(right);

						return Value{-val};
				 }

				case TokenType::NOT:
				 {if (!std::holds_alternative<bool>(right)){
								throw std::runtime_error("Invalid right expression for NOT");
						}
						
						bool val = std::get<bool>(right);

						return Value(!val);

				 }

				default:
						throw std::runtime_error("Unknown unary expression");
		}

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
		else if(const auto* ptr = dynamic_cast<const PrefixExpression*>(&node)){
				const auto right = eval(*ptr->right);
				return evalPrefixExpression(ptr->token,right);
		}
		else{
				throw std::runtime_error("Unrecognised ast");
		}
}

};	
