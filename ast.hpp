#pragma once

#include <memory>
#include "unique_ptr.hpp"
#include "shared_ptr.hpp"
#include "vector.hpp"
#include "optional.hpp"
#include <iostream>
#include <optional>
#include "token.hpp"
#include <string>
#include <vector>

class Node{

public:

		virtual ~Node() = default;
		virtual ricc::String TokenLiteral() const = 0;
		virtual void print() const = 0;

};


class Statement: public Node{
public:
		~Statement() override = default;
		
};


class Expression: public Node{

public:
		
		~Expression() override = default;
};

class Program: public Node{

public:

		ricc::Vector<ricc::unique_ptr<Statement>> statements;

		ricc::String TokenLiteral()const override{

				if (statements.empty()){

						return "";
				}
				return statements.front()->TokenLiteral();
			
		}
		void print() const override{
				statements.front()->print();
		};

};

class Identifier: public Expression{

		public:
				Token token;
				ricc::String value;

		Identifier(Token token, ricc::String value):token(std::move(token)),value(std::move(value))
		{
		}

		ricc::String TokenLiteral() const override{
				return token.text;
		}

		void print() const override{

				std::cout << token.text << std::endl;
		};

};

class VariableDeclaration: public Statement{

		
		public:
		Token token;
		ricc::unique_ptr<Identifier> name;
		ricc::unique_ptr<Expression> value;		


		VariableDeclaration(Token token):token(std::move(token)){

		}
				
		ricc::String TokenLiteral() const override{

				return token.text;
		};

		void print() const override{

				std::cout << token.text << std::endl;
				name->print();
				value->print();
		};


};


class Integer_Liter: public Expression{

		public:
				Token token;
				int value;

				Integer_Liter(Token token,int value):token(token),value(value){};

				ricc::String TokenLiteral() const override{

						return token.text;
				}
void print() const override{

		std::cout << value << std::endl;
};

};

class FloatLiter: public Expression{

		public:
				Token token;
				float value;

				FloatLiter(Token token,float value):token(token),value(value){};

				ricc::String TokenLiteral() const override{

						return token.text;
				}
void print() const override{

		std::cout << value << std::endl;
};

};

class Boolean: public Expression{

public:
		Token token;
		bool value;

		Boolean(Token token,bool value):token(token),value(value){};

		ricc::String TokenLiteral()const override{

				return token.text;
		}
void print() const override{

		std::cout << token.text << std::endl;
};

};

class Return: public Statement{

		public:
				Token token;
				ricc::unique_ptr<Expression> Returnvalue;

				Return(Token token):token(std::move(token)){}

				ricc::String TokenLiteral() const override{

						return token.text;
				}
void print() const override{

		std::cout << token.text << std::endl;
		Returnvalue->print();
};

};

/* a+ b + c for example, you start with a + b, (parse that) then check precedence for +c*/
class InfixExpression: public Expression{

public:

		Token token;
		ricc::unique_ptr<Expression> left;
		ricc::unique_ptr<Expression> right;

		InfixExpression(Token token,ricc::unique_ptr<Expression> left,ricc::unique_ptr<Expression> right):
				token(token),left(std::move(left)),right(std::move(right))
		{
		}

		ricc::String TokenLiteral() const override{

						return token.text;
				}

void print() const override{

		left->print();
		std::cout << token.text << std::endl;
		right->print();
};
};


class PrefixExpression: public Expression{

public:

		Token token;
		ricc::unique_ptr<Expression> right;

		PrefixExpression(Token token,ricc::unique_ptr<Expression> right):token(std::move(token)),right(std::move(right))
		{
		};

		ricc::String TokenLiteral() const override{

				return token.text;
		}
void print() const override{

		std::cout << token.text << std::endl;

		right->print();
};



};

class BlockStatements: public Statement{

public:

		ricc::Vector<ricc::unique_ptr<Statement>> Block_Statements;


		ricc::String TokenLiteral() const override{

				if (Block_Statements.empty()){

						return "empty";
				}
				return Block_Statements.front()->TokenLiteral();
		}
void print() const override{};

};


class IfStatement: public Statement{

public:

		Token token;
		ricc::unique_ptr<Expression> condition;
		ricc::unique_ptr<BlockStatements> Consequence;
		ricc::unique_ptr<BlockStatements> Alternative;


		IfStatement(Token token): token(std::move(token)){};

		ricc::String TokenLiteral() const override{

				return token.text;
		}
void print() const override{};

};

class FunctionParameters: public Node{
		public:
				Token binding_type;
				ricc::unique_ptr<Identifier> name;

		FunctionParameters(Token token,ricc::unique_ptr<Identifier> name):binding_type(std::move(token)),name(std::move(name)){};

		ricc::String TokenLiteral() const override{

				return binding_type.text;
		};
void print() const override{
};
};

class FunctionLiteral: public Statement{

public:

		Token token;
		ricc::unique_ptr<Identifier> name;
		ricc::Vector<ricc::unique_ptr<FunctionParameters>> Parameters;
		ricc::shared_ptr<BlockStatements> FunctionBody;
			TokenType type{TokenType::ILLEGAL};

		FunctionLiteral(Token token):token(std::move(token))
		{
		};

		ricc::String TokenLiteral() const override {

				return token.text;
		}
void print() const override{};

};

class CallExpression: public Expression{

public:

		Token token;
		ricc::unique_ptr<Expression> function_name;
		ricc::optional<ricc::Vector<ricc::unique_ptr<Expression>>> arguments;

		CallExpression(Token token, ricc::unique_ptr<Expression> function):token(token),function_name(std::move(function))
		{}

		ricc::String TokenLiteral() const override{

				return token.text;
		}

void print() const override{};

};

class ExpressionStatement: public Statement{

public:
		Token token;
		ricc::unique_ptr<Expression> expr;

		ExpressionStatement(Token token):token(std::move(token)) {
		}

		ricc::String TokenLiteral() const override{

				return expr ? expr->TokenLiteral() : "";
		}

		void print() const override{
		};
};
