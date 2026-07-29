#include "lexxer.hpp"
#include "ast.hpp"
#include "token.hpp"
#include <memory>
#include <utility>


class Parser{

private:
		lexxer l;
		Token curr_tok;
		Token next_tok;

		void nextToken(){

				curr_tok = std::move(next_tok);
				next_tok = l.nextToken();
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
		
		std::unique_ptr<Expression> ParseExpression(){

				switch (curr_tok.tokentype){
				
						case TokenType::INTEGER:
								return parseInteger();
						default:
								return nullptr;
				}
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

		std::unique_ptr<Return> parseReturn(){

				std::cout << "Entered return" << std::endl;
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


		Parser(lexxer lexer):l(std::move(lexer)),curr_tok(std::move(l.nextToken())),next_tok(std::move(l.nextToken()))
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


