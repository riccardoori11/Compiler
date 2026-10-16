#pragma once
#include "ast.hpp"
#include "object.hpp"
#include "token.hpp"
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>


using VectorStatement = std::vector<std::unique_ptr<Statement>>;

class Evaluator{

private:



public:

Evaluator() = default;

bool IsNumericValue(const Value& left, const Value& right){
 return((std::holds_alternative<int>(left) || std::holds_alternative<float>(left) || std::holds_alternative<double>(left)) && (std::holds_alternative<int>(right) || std::holds_alternative<float>(right)
	|| std::holds_alternative<double>(right)));
}

ExecutionResult evalStatement(const VectorStatement& statements){

		if (statements.empty()){

				throw std::runtime_error("Empty statements");
		}

		ExecutionResult result{};

		for (const auto& statement: statements){

				result = execute(*statement);

				if (result.is_return){

						return result;
				}
		}

		return result;
}

ExecutionResult execute(const Statement& node){

		if (const auto *ptr = dynamic_cast<const Return*>(&node)){

				if (!ptr->Returnvalue){

						throw std::runtime_error("Return must have value associated with it");
				}
				return ExecutionResult(eval(*ptr->Returnvalue),true);
		}
		else if(const auto *ptr = dynamic_cast<const ExpressionStatement*>(&node)){
				return ExecutionResult(eval(*ptr->expr),false);
		}

		else if(const auto *ptr = dynamic_cast<const Program*>(&node)){

				return evalStatement(ptr->statements);
		}

		else if(const auto *ptr = dynamic_cast<const BlockStatements*>(&node)){
				if (ptr->Block_Statements.empty()){
						return ExecutionResult{};
				}
				return ExecutionResult(evalStatement(ptr->Block_Statements));
		}
		else if(const auto *ptr = dynamic_cast<const IfStatement*>(&node)){
				return evalIf(*ptr);
		}

		else{
				throw std::runtime_error("Unrecognised AST");
		}

}




Value evalInfixExpression(const Token& op, const Value& left, const Value& right){
		switch (op.tokentype){

				case TokenType::PLUS:
						{
								if (!IsNumericValue(left, right)){

										throw std::runtime_error("Wrong left and right input for addition");
								}


								return std::visit([](const auto& a, const auto& b) -> Value{

												using LEFT_NORM = std::remove_cvref_t<decltype(a)>;
												using RIGHT_NORM = std::remove_cvref_t<decltype(b)>;

								if constexpr (std::is_same_v<LEFT_NORM, std::monostate> || std::is_same_v<RIGHT_NORM, std::monostate>) {
												throw std::runtime_error("Invalid NULL operator");
								}
								else{
												return Value(a + b);
												}
										},left,right);
								
						}
				case TokenType::MINUS:
						{
								if (!IsNumericValue(left, right)){

										throw std::runtime_error("Wrong left and right input for substraction");
								}


								return std::visit([](const auto& a, const auto& b) -> Value{

												using LEFT_NORM = std::remove_cvref_t<decltype(a)>;
												using RIGHT_NORM = std::remove_cvref_t<decltype(b)>;

								if constexpr (std::is_same_v<LEFT_NORM, std::monostate> || std::is_same_v<RIGHT_NORM, std::monostate>) {
												throw std::runtime_error("Invalid NULL operator");
								}
								else{
												return Value(a - b);
												}
										},left,right);
								
						}

				case TokenType::MULTIPLICATION:
						{
								if (!IsNumericValue(left, right)){

										throw std::runtime_error("Wrong left and right input for multiplication");
								}


								return std::visit([](const auto& a, const auto& b) -> Value{

												using LEFT_NORM = std::remove_cvref_t<decltype(a)>;
												using RIGHT_NORM = std::remove_cvref_t<decltype(b)>;

								if constexpr (std::is_same_v<LEFT_NORM, std::monostate> || std::is_same_v<RIGHT_NORM, std::monostate>) {
												throw std::runtime_error("Invalid NULL operator");
								}
								else{
												return Value(a * b);
												}
										},left,right);
								
						}
				case TokenType::DIVISION:
						{
								if (!IsNumericValue(left, right)){

										throw std::runtime_error("Wrong left and right input for division");
								}


								return std::visit([](const auto& a, const auto& b) -> Value{

												using LEFT_NORM = std::remove_cvref_t<decltype(a)>;
												using RIGHT_NORM = std::remove_cvref_t<decltype(b)>;

								if constexpr (std::is_same_v<LEFT_NORM, std::monostate> || std::is_same_v<RIGHT_NORM, std::monostate>) {
												throw std::runtime_error("Invalid NULL operator");
								}
								else{
												return Value(a / b);
												}
										},left,right);
								
						}

				case TokenType::LESS:
						{
								if (!IsNumericValue(left, right)){

										throw std::runtime_error("Wrong left and right input for comparaison (LESS THAN)");
								}


								return std::visit([](const auto& a, const auto& b) -> Value{

												using LEFT_NORM = std::remove_cvref_t<decltype(a)>;
												using RIGHT_NORM = std::remove_cvref_t<decltype(b)>;

								if constexpr (std::is_same_v<LEFT_NORM, std::monostate> || std::is_same_v<RIGHT_NORM, std::monostate>) {
												throw std::runtime_error("Invalid NULL operator");
								}
								else{
												return Value(a < b);
												}
										},left,right);
								
						}
				case TokenType::GREATER:
						{
								if (!IsNumericValue(left, right)){

										throw std::runtime_error("Wrong left and right input for comparaison (GREATER THAN)");
								}


								return std::visit([](const auto& a, const auto& b) -> Value{

												using LEFT_NORM = std::remove_cvref_t<decltype(a)>;
												using RIGHT_NORM = std::remove_cvref_t<decltype(b)>;

								if constexpr (std::is_same_v<LEFT_NORM, std::monostate> || std::is_same_v<RIGHT_NORM, std::monostate>) {
												throw std::runtime_error("Invalid NULL operator");
								}
								else{
												return Value(a > b);
												}
										},left,right);
								
						}
				case TokenType::COMPARE:
						{
								
								return std::visit([](const auto& a, const auto& b) -> Value{

												using LEFT_NORM = std::remove_cvref_t<decltype(a)>;
												using RIGHT_NORM = std::remove_cvref_t<decltype(b)>;

								if constexpr (std::is_same_v<LEFT_NORM, std::monostate> || std::is_same_v<RIGHT_NORM, std::monostate>) {
												throw std::runtime_error("Invalid NULL operator");
								}
								else{
												return Value(a == b);
												}
										},left,right);
								
						}
				default:
						throw std::runtime_error("Unknown operator for infix expression");

		}
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

bool isTruthy(const Value& value){

		if (!std::holds_alternative<bool>(value)){

				throw std::runtime_error("Condition of If/Else statement must be able to be evaluated to a boolean");
		}

		return std::get<bool>(value);

}
ExecutionResult evalIf(const IfStatement& node){


		const auto& condition = eval(*node.condition);

		const auto& consequence = isTruthy(condition);

		if (consequence){

				return execute(*node.Consequence);
		}
		else if (node.Alternative){

				return execute(*node.Alternative);
		}

		else{
				return ExecutionResult{};
		}
		

}

// need to add double

Value eval(const Node& node){

		if (const auto* ptr = dynamic_cast<const Integer_Liter*>(&node)){
				return Value{ptr->value};
		}
		else if(const auto* ptr = dynamic_cast<const Boolean*>(&node)){
				return Value{ptr->value};
		}
		else if(const auto* ptr = dynamic_cast<const FloatLiter*>(&node)){
				return Value{ptr->value};
		}
		else if(const auto* ptr = dynamic_cast<const Program*>(&node)){
				const auto result = evalStatement(ptr->statements);
				
				return result.value;
				
		}
		else if(const auto* ptr = dynamic_cast<const ExpressionStatement*>(&node)){
				return eval(*ptr->expr);
		}
		else if(const auto* ptr = dynamic_cast<const PrefixExpression*>(&node)){
				const auto right = eval(*ptr->right);
				return evalPrefixExpression(ptr->token,right);
		}
		else if (const auto* ptr = dynamic_cast<const InfixExpression*>(&node)){
				const auto left = eval(*ptr->left);

				const auto right = eval(*ptr->right);

				return evalInfixExpression(ptr->token,left,right);
		}
		else{
				throw std::runtime_error("Unrecognised ast");
		}
}

};	
