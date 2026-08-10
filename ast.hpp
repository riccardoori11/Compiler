#include <memory>
#include "token.hpp"
#include <string>
#include <vector>

class Node{

public:

		virtual ~Node() = default;
		virtual std::string TokenLiteral() const = 0;

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



};


class Integer_Liter: public Expression{

		public:
				Token token;
				int value;

				Integer_Liter(Token token,int value):token(token),value(value){};

				std::string TokenLiteral() const override{

						return token.text;
				}

};

class Boolean: public Expression{

public:
		Token token;
		bool value;

		Boolean(Token token,int value):token(token),value(value){};

		std::string TokenLiteral()const override{

				return token.text;
		}

};

class Return: public Statement{

		public:
				Token token;
				std::unique_ptr<Expression> Returnvalue;

				Return(Token token):token(std::move(token)){}

				std::string TokenLiteral() const override{

						return token.text;
				}

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

};


class PrefixExpression: public Expression{

public:

		Token token;
		std::unique_ptr<Expression> right;

		PrefixExpression(Token token,std::unique_ptr<Expression> right):token(std::move(token)),right(std::move(right))
		{
		};

		std::string TokenLiteral() const{

				return token.text;
		}



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

};

