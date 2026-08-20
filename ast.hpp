#include <memory>
#include <iostream>
#include <optional>
#include "token.hpp"
#include <string>
#include <vector>

class Node{

public:

		virtual ~Node() = default;
		virtual std::string TokenLiteral() const = 0;
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

		std::vector<std::unique_ptr<Statement>> statements;

		std::string TokenLiteral()const override{

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
				std::string value;

		Identifier(Token token, std::string value):token(std::move(token)),value(std::move(value))
		{
		}

		std::string TokenLiteral() const override{
				return token.text;
		}

		void print() const override{

				std::cout << token.text << std::endl;
		};

};

class VariableDeclaration: public Statement{

		
		public:
		Token token;
		std::unique_ptr<Identifier> name;
		std::unique_ptr<Expression> value;		


		VariableDeclaration(Token token):token(std::move(token)){

		}
				
		std::string TokenLiteral() const override{

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

				std::string TokenLiteral() const override{

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

		Boolean(Token token,int value):token(token),value(value){};

		std::string TokenLiteral()const override{

				return token.text;
		}
void print() const override{

		std::cout << token.text << std::endl;
};

};

class Return: public Statement{

		public:
				Token token;
				std::unique_ptr<Expression> Returnvalue;

				Return(Token token):token(std::move(token)){}

				std::string TokenLiteral() const override{

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
		std::unique_ptr<Expression> left;
		std::unique_ptr<Expression> right;

		InfixExpression(Token token,std::unique_ptr<Expression> left,std::unique_ptr<Expression> right):
				token(token),left(std::move(left)),right(std::move(right))
		{
		}

		std::string TokenLiteral() const override{

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
		std::unique_ptr<Expression> right;

		PrefixExpression(Token token,std::unique_ptr<Expression> right):token(std::move(token)),right(std::move(right))
		{
		};

		std::string TokenLiteral() const override{

				return token.text;
		}
void print() const override{

		std::cout << token.text << std::endl;

		right->print();
};



};

class BlockStatements: public Statement{

public:

		std::vector<std::unique_ptr<Statement>> Block_Statements;


		std::string TokenLiteral() const override{

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
		std::unique_ptr<Expression> condition;
		std::unique_ptr<BlockStatements> Consequence;
		std::unique_ptr<BlockStatements> Alternative;


		IfStatement(Token token): token(std::move(token)){};

		std::string TokenLiteral() const override{

				return token.text;
		}
void print() const override{};

};

class FunctionParameters: public Node{
		public:
				Token binding_type;
				std::unique_ptr<Identifier> name;

		FunctionParameters(Token token,std::unique_ptr<Identifier> name):binding_type(std::move(token)),name(std::move(name)){};

		std::string TokenLiteral() const override{

				return binding_type.text;
		};
void print() const override{
};
};

class FunctionLiteral: public Statement{

public:

		Token token;
		std::unique_ptr<Identifier> name;
		std::vector<std::unique_ptr<FunctionParameters>> Parameters;
		std::unique_ptr<BlockStatements> FunctionBody;

		FunctionLiteral(Token token):token(std::move(token))
		{
		};

		std::string TokenLiteral() const override {

				return token.text;
		}
void print() const override{};

};

class CallExpression: public Expression{

public:

		Token token;
		std::unique_ptr<Expression> function_name;
		std::optional<std::vector<std::unique_ptr<Expression>>> arguments;

		CallExpression(Token token, std::unique_ptr<Expression> function):token(token),function_name(std::move(function))
		{}

		std::string TokenLiteral() const override{

				return token.text;
		}
void print() const override{};

};

class ExpressionStatement: public Statement{

public:
		Token token;
		std::unique_ptr<Expression> expr;

		ExpressionStatement(Token token):token(std::move(token))
		{
		}

		std::string TokenLiteral() const override{

				return expr ? expr->TokenLiteral() : "";
		}

		void print() const override{
		};
};
