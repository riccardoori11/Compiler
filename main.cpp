#include "lexxer.hpp"
#include <format>
#include <assert.h>
#include "parser.hpp"
#include "token.hpp"
#include <ios>
#include <unistd.h>
#include <utility>

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
				case TokenType::LESS:
						return "LESS";
				case TokenType::DIVISION:
						return "DIVISION";
				case TokenType::GREATER:
						return "GREATER";
				case TokenType::NOT:
						return "NOT";
				case TokenType::LPARENT:
						return "Left_PARENTHESES";
				case TokenType::RPARENT:
						return "Right_PARENTHESES";
				case TokenType::IF:
						return "if";
				case TokenType::LBRAC:
						return "{";
				case TokenType::RBRAC:
						return "}";
				case TokenType::ELSE:
						return "else";
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
/*
		lexxer a{"int x = (5+5)*2"};

		Parser parser(std::move(a));

		auto program = parser.ParseProgram();

		auto declaration = dynamic_cast<VariableDeclaration*>(program->statements[0].get());

		auto name = declaration->name->TokenLiteral();
		

		auto multiplication = dynamic_cast<InfixExpression*>(declaration->value.get());
		

		auto five_plus_five = dynamic_cast<InfixExpression*>(multiplication->left.get());
		
		auto five = five_plus_five->left->TokenLiteral();
		
		auto second_five = five_plus_five->right->TokenLiteral();

		assert(declaration->TokenLiteral() == "int");
		assert(declaration->name->TokenLiteral() == "x");
		assert(multiplication->TokenLiteral() == "*");
		assert(five_plus_five->TokenLiteral() == "+");
		assert(five_plus_five->left->TokenLiteral() == "5");
		assert(five_plus_five->right->TokenLiteral() == "5");
*/
		lexxer c{"if (x + y){int x = 2} else{int x = 3};"};

		Parser parser1(std::move(c));
		auto program1 = parser1.ParseProgram();

		assert(program1->statements.size() == 1);

		auto declaration = dynamic_cast<IfStatement*>(program1->statements[0].get());
		std::cout << declaration->TokenLiteral() << std::endl;
		assert(declaration->TokenLiteral() == "if");

		auto condition = dynamic_cast<InfixExpression*>(declaration->condition.get());

		auto addition = condition->TokenLiteral();
		std::cout << addition << std::endl;

		auto first_var = condition->left->TokenLiteral();
		std::cout << first_var << std::endl;

		auto second_var = condition->right->TokenLiteral();
		std::cout << second_var << std::endl;

		assert(declaration->Consequence->Block_Statements.size() == 1);
		auto consequence = dynamic_cast<VariableDeclaration*>(declaration->Consequence->Block_Statements[0].get());

		assert(consequence->TokenLiteral() == "int");

		assert(consequence->name->TokenLiteral() == "x");

		assert(consequence->value->TokenLiteral() == "2");


	/*	

		

		auto addition = dynamic_cast<InfixExpression*>(declaration->value.get());

		auto one = dynamic_cast<Integer*>(addition->left.get());

		auto multiplication = dynamic_cast<InfixExpression*>(addition->right.get());

		auto two = dynamic_cast<Integer*>(multiplication->left.get());

		auto negative = dynamic_cast<PrefixExpression*>(multiplication->right.get());
		auto three = dynamic_cast<Integer*>(negative->right.get());


				std::cout << 
				std::format("{} {} = {} {} {} {} {}{}", 
				declaration->TokenLiteral(),name, one->TokenLiteral(), addition->TokenLiteral(), two->TokenLiteral(), multiplication->TokenLiteral(),negative->TokenLiteral(),three->TokenLiteral()) 
				<< std::endl;
*/
		return 0;
}
