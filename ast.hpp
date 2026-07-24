#include <algorithm>
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


class Integer: public Expression{

		public:
				Token token;
				int value;

				Integer(Token token,int value):token(token),value(value){};

				std::string TokenLiteral() const override{

						return token.text;
				}

};


