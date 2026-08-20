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
				case TokenType::COMMA:
						return ",";
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
		lexxer a{"int x = 4;"};

		Parser parser(std::move(a));

		auto program = parser.ParseProgram();

		auto p = program->statements[0].get();

		p->print();
/*
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
/*
		lexxer c{"if (x + y){int x = 2;} else{int x = 3;};"};

		Parser parser1(std::move(c));
		auto program1 = parser1.ParseProgram();

		assert(program1->statements.size() == 1);

		auto declaration1 = dynamic_cast<IfStatement*>(program1->statements[0].get());
		assert(declaration1->TokenLiteral() == "if");

		auto condition = dynamic_cast<InfixExpression*>(declaration1->condition.get());

		auto addition = condition->TokenLiteral();
		assert(addition == "+");

		auto first_var = condition->left->TokenLiteral();
		assert(first_var == "x");

		auto second_var = condition->right->TokenLiteral();
		assert(second_var == "y");

		assert(declaration1->Consequence->Block_Statements.size() == 1);
		auto consequence = dynamic_cast<VariableDeclaration*>(declaration1->Consequence->Block_Statements[0].get());

		assert(consequence->TokenLiteral() == "int");

		assert(consequence->name->TokenLiteral() == "x");

		assert(consequence->value->TokenLiteral() == "2");

		auto alter = dynamic_cast<VariableDeclaration*>(declaration1->Alternative->Block_Statements[0].get());

		assert(alter ->TokenLiteral() == "int");

		auto alter_name = alter->name->TokenLiteral();
		assert(alter_name == "x");

		auto alter_value = alter->value->TokenLiteral();

		assert(alter_value == "3");

		lexxer func("int x(int y){return y + 1;}");
		Parser parsefunc(std::move(func));
		
		auto program_func = parsefunc.ParseProgram();

		assert(program_func->statements.size() == 1);


		*/
/*
		lexxer d{"int declare(){ double x = 4; }"};

		Parser p1(std::move(d));

		auto program = p1.ParseProgram();

		auto function_decl = dynamic_cast<FunctionLiteral*>(program->statements[0].get());

		auto name = function_decl->name.get();

		assert(name->token.text == "declare");

		assert(function_decl->FunctionBody->Block_Statements.size() == 1);

		auto function_decl_function_body = dynamic_cast<VariableDeclaration*>(function_decl->FunctionBody->Block_Statements[0].get());

		assert(function_decl_function_body->TokenLiteral() == "double");

		assert(function_decl_function_body->name->TokenLiteral() == "x");

		assert(function_decl_function_body->value->TokenLiteral() == "4");
	*/	
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
