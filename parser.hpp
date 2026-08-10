#include "lexxer.hpp"
#include <assert.h>
#include "ast.hpp"
#include "token.hpp"
#include <algorithm>
#include <charconv>
#include <execution>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>


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


		bool isValidVariableType(TokenType type){

				switch (type){

						case TokenType::INT:
						case TokenType::DOUBLE:
						case TokenType::BOOL:
								return true;
						default:
								return false;
				}
		}

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

				char* begin = curr_tok.text.data();
				char* end = begin + curr_tok.text.size();
				int value2{};
				std::from_chars(begin,end,value2);
				return std::make_unique<Integer_Liter>(curr_tok,value2);
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

				std::cout << curr_tok.text << std::endl;

				std::unique_ptr<Expression> left;

				switch (curr_tok.tokentype){
				
						case TokenType::INTEGER:
								left = parseInteger();
								break;
						case TokenType::IDENTIFIER:
								std::cout << "Entered Identifier" << std::endl;
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
				std::cout << curr_tok.text << std::endl;
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

		std::unique_ptr<BlockStatements> parseBlockStatement(){

				std::cout << "Entering Block Statement" << std::endl;

				auto block_statement = std::make_unique<BlockStatements>();

				while (!current_type_is(TokenType::ENDOFFILE) && !current_type_is(TokenType::RBRAC)){

						auto statement = parseStatement();

						if (statement){

								block_statement->Block_Statements.push_back(std::move(statement));
						}
						nextToken();
				}

				return block_statement;
		}
		std::unique_ptr<IfStatement> parseIfStatement(){

				std::cout << "Parsing If Statement" << std::endl;
				std::cout << curr_tok.text << std::endl;

				auto declaration = std::make_unique<IfStatement>(curr_tok);

				if (!next_type_is(TokenType::LPARENT)){

						return nullptr;
				}

				nextToken();

				declaration->condition = ParseExpression(LOWEST);

				std::cout << "Finished parsing condition" << std::endl;
/*
				if (!next_type_is(TokenType::RPARENT)){
						std::cout << "Hi" << std::endl;

						return nullptr;
				}
*/
				nextToken();

				if (!current_type_is(TokenType::LBRAC)){

						std::cout << "Nullpointer" << std::endl;
						return nullptr;
				}

				declaration->Consequence = parseBlockStatement();
				std::cout << "Finished parsing Consequence" << std::endl;
				if (next_type_is(TokenType::ELSE)){
						std::cout << "parsing else" << std::endl;
						nextToken();

						if (!next_type_is(TokenType::LBRAC)){
						
								return nullptr;
						}
						std::cout << "parsing Alternative" << std::endl;
						declaration->Alternative = parseBlockStatement();
						std::cout << "Finished parsing Alternative" << std::endl;
				}

				
				nextToken();

				return declaration;
		}

		/*Might be empty parameter*/
		std::optional<std::vector<std::unique_ptr<FunctionParameters>>> parseFunctionParameters(){

				std::vector<std::unique_ptr<FunctionParameters>> parameters;

				if (next_type_is(TokenType::RPARENT)){

						nextToken();
						return std::vector<std::unique_ptr<FunctionParameters>>{};
				}

				while (true){

						nextToken();

						if (!isValidVariableType(curr_tok.tokentype)){

								return std::nullopt;
						}
						auto bindType = curr_tok;

						nextToken();

						if (!current_type_is(TokenType::IDENTIFIER)){

								return std::nullopt;
						}

						auto name = std::make_unique<Identifier>(curr_tok,curr_tok.text);

						parameters.push_back(std::make_unique<FunctionParameters>(std::move(bindType),std::move(name)));

						if (next_type_is(TokenType::RPARENT)){

								nextToken();
								break;
						}
						/*Invalid call*/
						if (!next_type_is(TokenType::COMMA)){
								return std::nullopt;

						}

						nextToken();
				}

				return std::optional<std::vector<std::unique_ptr<FunctionParameters>>>(std::move(parameters));

		}

		std::unique_ptr<Statement> parseFunctionLit(){

				auto declaration = std::make_unique<FunctionLiteral>(curr_tok);

				nextToken();

				if (!current_type_is(TokenType::IDENTIFIER)){

						return nullptr;
				}

				if (!next_type_is(TokenType::LPARENT)){

						return nullptr;
				}
				nextToken();
				assert(curr_tok.tokentype == TokenType::LPARENT);

				auto parameters = parseFunctionParameters();

				if (!parameters){
						throw std::runtime_error("Could not parse function parameters");
				}

				declaration->Parameters = std::move(*parameters);

				if (!next_type_is(TokenType::LBRAC)){

						throw std::runtime_error("Eror parsing beginning function body");
				}

				nextToken();
				declaration->FunctionBody = parseBlockStatement();

				nextToken();
				return declaration;

		}
		std::unique_ptr<Statement> parseStatement(){

				switch (curr_tok.tokentype){

						case TokenType::INT:
								if (next_type_is(TokenType::IDENTIFIER)){

										return parseVariableDeclaration();
								}
								else if(next_type_is(TokenType::LPARENT)){
										return parseFunctionLit();
								}
						case TokenType::DOUBLE:
								return parseVariableDeclaration();
						case TokenType::BOOL:
								return parseVariableDeclaration();
						case TokenType::RETURN:
								return parseReturn();
						case TokenType::IF:
								return parseIfStatement();
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


