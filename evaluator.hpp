#pragma once
#include "ast.hpp"
#include "object.hpp"
#include "token.hpp"
#include "Environment.hpp"
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

/*
 * for now having one global env for single scopes
 * */

using VectorStatement = std::vector<std::unique_ptr<Statement>>;
template<typename T>
constexpr bool is_numeric = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

template<typename T>
constexpr bool is_BOOL = std::is_same_v<T, bool>;

class Evaluator{

private:

		Environment env;


public:

Evaluator() = default;


bool MatchesType(TokenType type, const Value& val){

		static const std::unordered_map<TokenType, std::size_t> map{
				{TokenType::INT, Value{int{}}.index()},
				{TokenType::DOUBLE, Value{double{}}.index()},
				{TokenType::BOOL, Value{bool{}}.index()},
				{TokenType::FLOAT, Value{float{}}.index()}
		};

		const auto it = map.find(type);
		return it != map.end() && val.index() == it->second;
}

ExecutionResult evalStatement(const VectorStatement& statements, Environment& env){

		if (statements.empty()){

				throw std::runtime_error("Empty statements");
		}

		ExecutionResult result{};

		for (const auto& statement: statements){

				result = execute(*statement,env);

				if (result.is_return){

						return result;
				}
		}

		return result;
}

ExecutionResult execute(const Statement& node, Environment& env){

		if (const auto *ptr = dynamic_cast<const Return*>(&node)){

				if (!ptr->Returnvalue){

						throw std::runtime_error("Return must have value associated with it");
				}
				return ExecutionResult(eval(*ptr->Returnvalue,env),true);
		}
		else if(const auto *ptr = dynamic_cast<const ExpressionStatement*>(&node)){
				return ExecutionResult(eval(*ptr->expr,env),false);
		}
		else if(const auto *ptr = dynamic_cast<const FunctionLiteral*>(&node)){
				FunctionObj func{};
				func.function_type = ptr->type; 
				func.env = &env;

				for (const auto& param: ptr->Parameters){
						func.parameters.push_back(Parameter(
												param->name->value,
												param->binding_type.tokentype
												));
				}

				func.Body = ptr->FunctionBody.get();

				Binding binding{
						TokenType::FUNCTION,
						std::make_shared<FunctionObj>(std::move(func))
				};

				env.define(ptr->name->value, binding);
				return ExecutionResult{};

		}
		else if(const auto *ptr = dynamic_cast<const VariableDeclaration*>(&node)){
				Binding var{};
				var.type = ptr->token.tokentype;
				if (ptr->value){

						var.value = eval(*ptr->value,env);
				}
				if (var.value && !MatchesType(var.type, *var.value )){
						throw std::runtime_error("Wrong type mismatch");
				}

				env.define(ptr->name->value, var);
				return ExecutionResult{};
		}

		else if(const auto *ptr = dynamic_cast<const Program*>(&node)){

				return evalStatement(ptr->statements,env);
		}

		else if(const auto *ptr = dynamic_cast<const BlockStatements*>(&node)){
				if (ptr->Block_Statements.empty()){
						return ExecutionResult{};
				}
				return ExecutionResult(evalStatement(ptr->Block_Statements,env));
		}
		else if(const auto *ptr = dynamic_cast<const IfStatement*>(&node)){
				return evalIf(*ptr, env);
		}

		else{
				throw std::runtime_error("Unrecognised AST");
		}

}




Value evalInfixExpression(const Token& op, const Value& left, const Value& right){

		return std::visit([&](const auto& a, const auto& b) -> Value{

				using A = std::remove_cvref_t<decltype(a)>; 
				using B = std::remove_cvref_t<decltype(b)>;

				if constexpr (is_numeric<A> && is_numeric<B>) {
						
						switch (op.tokentype){

								case TokenType::PLUS:
										return Value(a+b);
								
								case TokenType::MINUS:
										return Value(a-b);

								case TokenType::MULTIPLICATION:
										return Value(a*b);
								case TokenType::DIVISION:
										return Value(a / b);

								case TokenType::COMPARE:
										return Value(a == b);

								case TokenType::LESS:
										return Value(a < b);

								case TokenType::GREATER:
										return Value(a > b);
								default:
										throw std::runtime_error("unknown token type");
						}
				}
				else if constexpr (is_BOOL<A> && is_BOOL<B>) {
						if (op.tokentype == TokenType::COMPARE){
								return Value(a == b);
						}
						else{
								throw std::runtime_error("Invalid BOOLEAN COMPARAISON");
						}
				}
				else{
						throw std::runtime_error("Invalid numeric value for infix expression");
				}

						},left,right);
		
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


ExecutionResult evalIf(const IfStatement& node, Environment& env){

		const auto& condition = eval(*node.condition,env);

		const auto& consequence = isTruthy(condition);

		if (consequence){

				return execute(*node.Consequence, env);
		}
		else if (node.Alternative){

				return execute(*node.Alternative,env);
		}

		else{
				return ExecutionResult{};
		}
}

Value evalCallExpression(const CallExpression* node, Environment& env){

		Value callee = eval(*node->function_name,env);

		auto function = std::get<std::shared_ptr<FunctionObj>>(callee);

		if (!function){

				throw std::runtime_error("null function");
		}

		if ((node->arguments->size() != function->parameters.size())){

				throw std::runtime_error("callee must have the same number of arguments as the function it is calling");
		}

		ExecutionResult res = execute(*function->Body,env);

		if (!res.is_return){
				throw std::runtime_error("nothing was returned");
		}

		return res.value;

}

// need to add double


Value eval(const Node& node, Environment& env){

		if (const auto* ptr = dynamic_cast<const Integer_Liter*>(&node)){
				return Value{ptr->value};
		}
		else if(const auto* ptr = dynamic_cast<const Boolean*>(&node)){
				return Value{ptr->value};
		}
		else if(const auto* ptr = dynamic_cast<const FloatLiter*>(&node)){
				return Value{ptr->value};
		}
		else if(const auto* ptr = dynamic_cast<const CallExpression*>(&node)){
				return evalCallExpression(ptr, env); 
		}
		else if(const auto* ptr = dynamic_cast<const Identifier*>(&node)){
				auto bind =	env.Get(ptr->value);

				if (!bind){

						throw std::runtime_error("could not find name");
				}
				else if(!bind->value){
						throw std::runtime_error("no value associated with it");
				}
				return bind->value.value();
		}
		else if(const auto* ptr = dynamic_cast<const Program*>(&node)){
				const auto result = evalStatement(ptr->statements,env);

				if (result.is_return){

						throw std::runtime_error("Return should be in function");
				}

				return result.value;
				
		}
		else if(const auto* ptr = dynamic_cast<const ExpressionStatement*>(&node)){
				return eval(*ptr->expr,env);
		}
		else if(const auto* ptr = dynamic_cast<const PrefixExpression*>(&node)){
				const auto right = eval(*ptr->right,env);
				return evalPrefixExpression(ptr->token,right);
		}
		else if (const auto* ptr = dynamic_cast<const InfixExpression*>(&node)){
				const auto left = eval(*ptr->left,env);

				const auto right = eval(*ptr->right,env);

				return evalInfixExpression(ptr->token,left,right);
		}
		else{
				throw std::runtime_error("Unrecognised ast");
		}
}

};	
