#pragma once
#include "ast.hpp"
#include "object.hpp"
#include "token.hpp"
#include "Environment.hpp"
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

using VectorStatement = std::vector<std::unique_ptr<Statement>>;
template<typename T>
constexpr bool is_numeric = std::is_arithmetic_v<T> && !std::is_same_v<T, bool>;

template<typename T>
constexpr bool is_BOOL = std::is_same_v<T, bool>;

class Evaluator{

private:

		std::unordered_set<std::shared_ptr<EnvironmentFrame>> captured_environments;

    static void validateIntegerInfix(TokenType op, int a, int b){
        constexpr int minimum = std::numeric_limits<int>::min();
        constexpr int maximum = std::numeric_limits<int>::max();
        bool overflow = false;

        switch (op){
            case TokenType::PLUS:
                overflow = (b > 0 && a > maximum - b)
                    || (b < 0 && a < minimum - b);
                break;
            case TokenType::MINUS:
                overflow = (b > 0 && a < minimum + b)
                    || (b < 0 && a > maximum + b);
                break;
            case TokenType::MULTIPLICATION:
                overflow = (a > 0 && b > 0 && a > maximum / b)
                    || (a > 0 && b < 0 && b < minimum / a)
                    || (a < 0 && b > 0 && a < minimum / b)
                    || (a < 0 && b < 0 && a < maximum / b);
                break;
            case TokenType::DIVISION:
                overflow = a == minimum && b == -1;
                break;
            default:
                return;
        }

        if (overflow){
            throw std::runtime_error("Integer overflow");
        }
    }


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

				if (!statement){
						throw std::runtime_error("Invalid null statement");
				}
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
				if (!ptr->expr){
						throw std::runtime_error("Invalid expression statement");
				}
				return ExecutionResult(eval(*ptr->expr,env),false);
		}
		else if(const auto *ptr = dynamic_cast<const FunctionLiteral*>(&node)){

				if (!ptr->name || !ptr->FunctionBody){
						throw std::runtime_error("Invalid function declaration");
				}
				FunctionObj func{};
				func.function_type = ptr->type; 
				captured_environments.insert(env.frame);
				func.env = env.frame;

				for (const auto& param: ptr->Parameters){
						if (!param || !param->name){
								throw std::runtime_error("Invalid function parameter");
						}
						func.parameters.push_back(Parameter(
												param->name->value,
												param->binding_type.tokentype
												));
				}

				func.block = ptr->FunctionBody;

				Binding binding{
						TokenType::FUNCTION,
						std::make_shared<FunctionObj>(std::move(func))
				};

				env.define(ptr->name->value, binding);
				return ExecutionResult{};

		}
		else if(const auto *ptr = dynamic_cast<const VariableDeclaration*>(&node)){
				if (!ptr->name){
						throw std::runtime_error("Invalid variable declaration");
				}
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
				Environment block_env(&env);
				return evalStatement(ptr->Block_Statements,block_env);
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
						if constexpr (std::is_same_v<A, int> && std::is_same_v<B, int>){
								validateIntegerInfix(op.tokentype, a, b);
						}

						switch (op.tokentype){

								case TokenType::PLUS:
										return Value(a+b);
								case TokenType::MINUS:
										return Value(a-b);
								case TokenType::MULTIPLICATION:
										return Value(a*b);
								case TokenType::DIVISION:
										if (b == 0){
												throw std::runtime_error("Can't divide by zero");
										}
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
						if (val == std::numeric_limits<int>::min()){
								throw std::runtime_error("Integer overflow");
						}

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

		if (!node.condition || !node.Consequence){
				throw std::runtime_error("Invalid if statement");
		}
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

		if (!node || !node->function_name || !node->arguments){
				throw std::runtime_error("Invalid call expression");
		}
		Value callee = eval(*node->function_name,env);

		const auto* callable = std::get_if<std::shared_ptr<FunctionObj>>(&callee);
		if (!callable){
				throw std::runtime_error("Value is not callable");
		}
		auto function = *callable;

		if (!function){

				throw std::runtime_error("null function");
		}
		if (!function->block){
				throw std::runtime_error("Function has no body");
		}

		if ((node->arguments->size() != function->parameters.size())){

				throw std::runtime_error("callee must have the same number of arguments as the function it is calling");
		}

		auto defining_env = function->env.lock();
		if (!defining_env){
				throw std::runtime_error("Function environment has expired");
		}
		Environment call_env(std::move(defining_env));

		for (std::size_t i{}; i < function->parameters.size(); ++i){

				const auto& arg = function->parameters[i];

				if (!node->arguments->at(i)){
						throw std::runtime_error("Invalid call argument");
				}
				Value val = eval(*node->arguments->at(i), env);

				if (!MatchesType(arg.parameter_type,val)){
						throw std::runtime_error("Wrong Call parameter type");
				}

				call_env.define(arg.name,
								Binding{arg.parameter_type,val}
								);
		}

		// The outer function body shares the parameter scope. Nested lexical
		// blocks still create child scopes through execute().
		ExecutionResult res{};
		if (!function->block->Block_Statements.empty()){
				res = evalStatement(function->block->Block_Statements,call_env);
		}

		if (!res.is_return){
				throw std::runtime_error("nothing was returned");
		}

		if (!MatchesType(function->function_type, res.value)){
				throw std::runtime_error("Wrong return type");
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
				if (!ptr->expr){
						throw std::runtime_error("Invalid expression statement");
				}
				return eval(*ptr->expr,env);
		}
		else if(const auto* ptr = dynamic_cast<const PrefixExpression*>(&node)){
				if (!ptr->right){
						throw std::runtime_error("Invalid prefix operand");
				}
				const auto right = eval(*ptr->right,env);
				return evalPrefixExpression(ptr->token,right);
		}
		else if (const auto* ptr = dynamic_cast<const InfixExpression*>(&node)){
				if (!ptr->left || !ptr->right){
						throw std::runtime_error("Invalid infix operands");
				}
				const auto left = eval(*ptr->left,env);

				const auto right = eval(*ptr->right,env);

				return evalInfixExpression(ptr->token,left,right);
		}
		else{
				throw std::runtime_error("Unrecognised ast");
		}
}

};	
