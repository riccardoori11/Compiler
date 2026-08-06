#include "lexxer.hpp"
#include "ast.hpp"
#include "token.hpp"
#include <memory>
#include <utility>


enum Precedence{

		LOWEST,
		COMPARE,
		LESSGREATER,
		SUM,
		PRODUCT,
		PREFIX,

};
class Parser{

private:
		lexxer l;
		Token curr_tok;
		Token next_tok;

		void nextToken(){

				curr_tok = std::move(next_tok);
				next_tok = l.nextToken();
		}

		Precedence getCurrentPrecedence(){

				return getPrecedence(curr_tok.tokentype); 
		}
		Precedence getPeekPrecedence(){

				return getPrecedence(next_tok.tokentype); 
		}


		Precedence getPrecedence(TokenType type){

				switch(type){
						case TokenType::COMPARE:
								return Precedence::COMPARE;
						case TokenType::LESS:
								return Precedence::LESSGREATER;
						case TokenType::GREATER:
								return Precedence::LESSGREATER;
						case TokenType::MULTIPLICATION:
								return Precedence::PRODUCT;
						case TokenType::PLUS:
								return Precedence::SUM;
						case TokenType::MINUS:
								return Precedence::SUM;
						case TokenType::DIVISION:
								return Precedence::PRODUCT;
						default:
								return LOWEST;
						
				}
		}

		bool current_type_is(TokenType type){

				return curr_tok.tokentype == type;
		}

		bool next_type_is(TokenType type){

				return next_tok.tokentype == type;
		}

		std::unique_ptr<Expression> parseInteger(){

				int value = std::stoi(curr_tok.text);
				return std::make_unique<Integer>(curr_tok,value);
		}

		std::unique_ptr<Expression> parseIdentifier(){

				std::string value = curr_tok.text;

				return std::make_unique<Identifier>(curr_tok,value);
		}

		std::unique_ptr<Expression> ParseGroupedExpression(){

				
				std::cout << "Eneterd" << std::endl;
				std::cout << curr_tok.text << std::endl;
				nextToken();

				auto right = ParseExpression(Precedence::LOWEST);

				if (!right){

						std::cout << "Entered ParseGroupedExpression" << std::endl;
						return nullptr;
				}

				if (!next_type_is(TokenType::RPARENT)){

						return nullptr;
				}

				nextToken();
				std::cout << curr_tok.text << std::endl;
				return right;
				
		}
		
		std::unique_ptr<Expression> ParseExpression(Precedence precedence = Precedence::LOWEST){

				std::unique_ptr<Expression> left;

				switch (curr_tok.tokentype){
				
						case TokenType::INTEGER:
								left = parseInteger();
								break;
						case TokenType::IDENTIFIER:
								left = parseIdentifier();
								break;
						case TokenType::NOT:
								left = parsePrefix();
								break;
						case TokenType::MINUS:
								left = parsePrefix();
								break;
						case TokenType::LPARENT:
								std::cout << "Enterinf Parsed Group Expression" << std::endl;
								left = ParseGroupedExpression();
								break;

						default:
								return nullptr;
				}

				/*Keep extending the left*/

		while(!next_type_is(TokenType::SEMICOLON) && precedence < getPeekPrecedence()){
				switch (next_tok.tokentype){
						case TokenType::COMPARE:
						case TokenType::GREATER:
						case TokenType::MULTIPLICATION:
						case TokenType::LESS:
						case TokenType::DIVISION:
						case TokenType::PLUS:
						case TokenType::MINUS:

						nextToken();
						left = parseInfix(std::move(left));
						if (!left){

								return nullptr;
						}
						break;
						default:
								return left;

				}
		}
		return left;
		}


		std::unique_ptr<VariableDeclaration> parseVariableDeclaration(){

				auto declaration = std::make_unique<VariableDeclaration>(curr_tok);
				if (!next_type_is(TokenType::IDENTIFIER)){

						return nullptr;
				}
				nextToken();

				declaration->name = std::make_unique<Identifier>(curr_tok,curr_tok.text);

				if (!next_type_is(TokenType::EQUAL)){

						if (next_type_is(TokenType::SEMICOLON)){

								declaration->value = 0;
						}
						else{

								return nullptr;
						}
						
				}
				nextToken();
				nextToken();

				declaration->value = ParseExpression();

				
				nextToken();
				return declaration;


		}


		std::unique_ptr<Expression> parseInfix(std::unique_ptr<Expression> left){

				Token op = curr_tok;

				Precedence p = getCurrentPrecedence();

				nextToken();

				auto right = ParseExpression(p);

				if (!right){

						return nullptr;
				}
				return std::make_unique<InfixExpression>(std::move(op),std::move(left),std::move(right));
		}

		std::unique_ptr<Expression> parsePrefix(){
				Token op = curr_tok;
				nextToken();
				/*
				 * So we dont treat - as sum
				 * */
				auto right = ParseExpression(Precedence::PREFIX);

				return std::make_unique<PrefixExpression>(std::move(op),std::move(right));
		}
		

		std::unique_ptr<Return> parseReturn(){

				auto declaration = std::make_unique<Return>(curr_tok);
				if (next_type_is(TokenType::SEMICOLON)){

						return nullptr;
				}
				nextToken();

				declaration->Returnvalue = ParseExpression();

				nextToken();
				return declaration;
				
		}
		std::unique_ptr<Statement> parseStatement(){

				switch (curr_tok.tokentype){

						case TokenType::INT:
								return parseVariableDeclaration();
						case TokenType::DOUBLE:
								return parseVariableDeclaration();
						case TokenType::BOOL:
								return parseVariableDeclaration();
						case TokenType::RETURN:
								return parseReturn();
						default:
								return nullptr;
				}
		}

public:


		Parser(lexxer lexer):l(std::move(lexer)),curr_tok(l.nextToken()),next_tok(l.nextToken())
		{

		}
		std::unique_ptr<Program> ParseProgram(){

				auto program = std::make_unique<Program>();

				while (!current_type_is(TokenType::ENDOFFILE)){

						auto statement = parseStatement(); 
						if (statement){

								program->statements.push_back(std::move(statement));
								
						}
						nextToken();
				}

				return program;

		};


};


