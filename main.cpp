#include "lexxer.hpp"
#include "parser.hpp"
#include <ios>
#include <unistd.h>

std::string Convert_type_to_str(Token t){

		switch (t.tokentype){
				case TokenType::ENDOFFILE:
						return "END OF FILE";
				case TokenType::PLUS:
						return "PLUS";
				case TokenType::INTEGER:
						return "INTEGER";
				case TokenType::IDENTIFIER:
						return "IDENTIFIER";
				case TokenType::SEMICOLON:
						return "SEMICOLON";
				case TokenType::EQUAL:
						return "EQUAL";
				case TokenType::MINUS:
						return "MINUS";
				case TokenType::MULTIPLICATION:
						return "MULTIPLY";
				case TokenType::ILLEGAL:
						return "ILLEGAL";
				case TokenType::INT:
						return "INTEGER";
				case TokenType::BOOL:
						return "BOOL";
				case TokenType::DOUBLE:
						return "DOUBLE";
				case TokenType::COMPARE:
						return "COMPARE";
		}
		return "";
		
}

auto printToken(lexxer lex){

		while(true){

				Token token = lex.nextToken();

				std::cout << Convert_type_to_str(token) << " {"<< token.text << "} " << std::endl;
				if (token.tokentype == TokenType::ENDOFFILE){

						break;
				}
		}
}

int main(){

		lexxer a{"int x = 5;"};
		Parser p{std::move(a)};
		
		auto program = p.ParseProgram();

		std::cout << program->statements.size() << std::endl;

		auto *decalaration = dynamic_cast<VariableDeclaration*>(program->statements[0].get());

		std::cout << decalaration->token.text << std::endl;
		std::cout << decalaration->name->value << std::endl;
		
		auto *integer = dynamic_cast<Integer*>(decalaration->value.get());

		std::cout << integer->value << std::endl;


		return 0;
}



