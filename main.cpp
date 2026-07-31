#include "lexxer.hpp"
#include <format>
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
				case TokenType::RETURN:
						return "RETURN";
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

// change this later
auto printVariableDeclaration(std::unique_ptr<Program> p){


		for (std::size_t i{}; i < p->statements.size(); ++i){
		auto dec = dynamic_cast<VariableDeclaration*>(p->statements[i].get());

		auto name = dec->name->TokenLiteral();

		auto value = dec->value->TokenLiteral();

		std::cout <<"Type: "<< dec->TokenLiteral() <<  "\nName: " << name << "\nValue: " << value << std::endl;
		}
}


int main(){

		lexxer a{"int x = 1 + 2 * 3"};

		Parser parser(std::move(a));
		
		auto program = parser.ParseProgram();

		auto declaration = dynamic_cast<VariableDeclaration*>(program->statements[0].get());

		auto name = declaration->name->TokenLiteral();

		auto addition = dynamic_cast<InfixExpression*>(declaration->value.get());

		auto one = dynamic_cast<Integer*>(addition->left.get());

		auto multiplication = dynamic_cast<InfixExpression*>(addition->right.get());

		auto two = dynamic_cast<Integer*>(multiplication->left.get());

		auto three = dynamic_cast<Integer*>(multiplication->right.get());

				std::cout << 
				std::format("{} {} = {} {} {} {} {}", 
				declaration->TokenLiteral(),name, one->TokenLiteral(), addition->TokenLiteral(), two->TokenLiteral(), multiplication->TokenLiteral(),three->TokenLiteral()) 
				<< std::endl;

		return 0;
}
