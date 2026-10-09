#include "lexxer.hpp"
#include <format>
#include <assert.h>
#include "parser.hpp"
#include "token.hpp"
#include <ios>
#include <istream>
#include <stdexcept>
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

auto Parse_input(std::istream& input_stream, std::string& s){

		std::getline(input_stream,s);
		lexxer a{s};

		Parser parser(std::move(a));

		auto program = parser.ParseProgram();

		if (!program->statements.size()){

				throw std::runtime_error("Could not parse input");
		}

		auto p = program->statements[0].get();

		p->print();
}


int main(){

		// Regression test for error.md #1: an uninitialized declaration
		// must preserve the following declaration.
		{
				Parser parser(lexxer{"int x; int y = 2;"});
				auto program = parser.ParseProgram();
				assert(!program->statements.empty());
				auto declaration = dynamic_cast<VariableDeclaration*>(program->statements[0].get());
				assert(declaration != nullptr);
				assert(declaration->name->TokenLiteral() == "x");
				assert(declaration->value == nullptr && "int x; must have no initializer");
				assert(program->statements.size() == 2 && "The following declaration must remain a separate statement");
				auto second_declaration = dynamic_cast<VariableDeclaration*>(program->statements[1].get());
				assert(second_declaration != nullptr);
				assert(second_declaration->TokenLiteral() == "int");
				assert(second_declaration->name->TokenLiteral() == "y");
				assert(second_declaration->value != nullptr);
				assert(second_declaration->value->TokenLiteral() == "2");
		}
		// Regression tests for error.md #2: malformed expressions must
		// be rejected, not stored as apparently successful AST statements.
		
		{
				const char* invalid_inputs[] = {
						"int x = ;",
						"return @;",
						"int x = -;",
						"@",
						"1 + ;",
						"if (1 { int x = 2; }",
						"f(1, 2;"
				};
				bool all_rejected = true;
				for (const char* input : invalid_inputs){
						bool rejected = false;
						try {
								Parser parser(lexxer{input});
								auto program = parser.ParseProgram();
								rejected = program && program->statements.empty();
						} catch (const std::runtime_error&) {
								rejected = true;
						}
						if (!rejected){
								std::cerr << "FAIL: malformed input returned AST statements: " << input << '\n';
								all_rejected = false;
						}
				}
				assert(all_rejected && "Malformed expressions must not produce successful AST statements");
		}

		// Regression tests for error.md #3: block statements must leave
		// the next statement and the enclosing block's closing brace intact.
		{
				auto is_declaration = [](const Statement* statement, const char* name, int value){
						auto declaration = dynamic_cast<const VariableDeclaration*>(statement);
						if (!declaration || declaration->TokenLiteral() != "int" || !declaration->name ||
								declaration->name->TokenLiteral() != name){
								return false;
						}
						auto integer = dynamic_cast<const Integer_Liter*>(declaration->value.get());
						return integer && integer->value == value;
				};
				auto is_if = [](const Statement* statement, const char* condition,
								std::size_t body_size, bool has_else = false){
						auto conditional = dynamic_cast<const IfStatement*>(statement);
						return conditional && conditional->condition &&
								conditional->condition->TokenLiteral() == condition &&
								conditional->Consequence &&
								conditional->Consequence->Block_Statements.size() == body_size &&
								(has_else ? conditional->Alternative && conditional->Alternative->Block_Statements.empty()
										: conditional->Alternative == nullptr);
				};
				auto is_function = [](const Statement* statement, const char* name, std::size_t body_size){
						auto function = dynamic_cast<const FunctionLiteral*>(statement);
						return function && function->name && function->name->TokenLiteral() == name &&
								function->Parameters.empty() && function->FunctionBody &&
								function->FunctionBody->Block_Statements.size() == body_size;
				};
				auto is_return = [](const Statement* statement, const char* value){
						auto return_statement = dynamic_cast<const Return*>(statement);
						return return_statement && return_statement->Returnvalue &&
								return_statement->Returnvalue->TokenLiteral() == value;
				};

				bool all_blocks_preserved = true;
				auto test_block = [&](const char* name, const char* input, auto check){
						try {
								Parser parser(lexxer{input});
								auto program = parser.ParseProgram();
								if (program && check(*program)){
										return;
								}
								std::cerr << "FAIL: error.md #3 (" << name << "): unexpected AST: " << input << '\n';
						} catch (const std::runtime_error& error) {
								std::cerr << "FAIL: error.md #3 (" << name << "): " << error.what()
										<< ": " << input << '\n';
						}
						all_blocks_preserved = false;
				};

				test_block("declaration after if", "if (1) {} int y = 2;", [&](const Program& program){
						return program.statements.size() == 2 &&
								is_if(program.statements[0].get(), "1", 0) &&
								is_declaration(program.statements[1].get(), "y", 2);
				});

				test_block("declaration after if/else", "if (1) {} else {} int y = 2;", [&](const Program& program){
						return program.statements.size() == 2 &&
								is_if(program.statements[0].get(), "1", 0, true) &&
								is_declaration(program.statements[1].get(), "y", 2);
				});

				test_block("declaration after function", "int f() { return 1; } int y = 2;", [&](const Program& program){
						if (program.statements.size() != 2 || !is_function(program.statements[0].get(), "f", 1) ||
								!is_declaration(program.statements[1].get(), "y", 2)){
								return false;
						}
						auto function = dynamic_cast<const FunctionLiteral*>(program.statements[0].get());
						return is_return(function->FunctionBody->Block_Statements[0].get(), "1");
				});

				test_block("nested if preserves function scope", "int f() { if (1) {} } int y = 2;", [&](const Program& program){
						if (program.statements.size() != 2 || !is_function(program.statements[0].get(), "f", 1) ||
								!is_declaration(program.statements[1].get(), "y", 2)){
								return false;
						}
						auto function = dynamic_cast<const FunctionLiteral*>(program.statements[0].get());
						return is_if(function->FunctionBody->Block_Statements[0].get(), "1", 0);
				});

				test_block("nested if/else preserves function scope", "int f() { if (1) {} else {} } int y = 2;", [&](const Program& program){
						if (program.statements.size() != 2 || !is_function(program.statements[0].get(), "f", 1) ||
								!is_declaration(program.statements[1].get(), "y", 2)){
								return false;
						}
						auto function = dynamic_cast<const FunctionLiteral*>(program.statements[0].get());
						return is_if(function->FunctionBody->Block_Statements[0].get(), "1", 0, true);
				});

				test_block("statements after if inside function", "int f() { if (1) {} int x = 3; return x; } int y = 2;", [&](const Program& program){
						if (program.statements.size() != 2 || !is_function(program.statements[0].get(), "f", 3) ||
								!is_declaration(program.statements[1].get(), "y", 2)){
								return false;
						}
						auto function = dynamic_cast<const FunctionLiteral*>(program.statements[0].get());
						const auto& body = function->FunctionBody->Block_Statements;
						return is_if(body[0].get(), "1", 0) && is_declaration(body[1].get(), "x", 3) &&
								is_return(body[2].get(), "x");
				});

				test_block("nested if preserves outer if scope", "if (1) { if (2) {} } int y = 2;", [&](const Program& program){
						if (program.statements.size() != 2 || !is_if(program.statements[0].get(), "1", 1) ||
								!is_declaration(program.statements[1].get(), "y", 2)){
								return false;
						}
						auto conditional = dynamic_cast<const IfStatement*>(program.statements[0].get());
						return is_if(conditional->Consequence->Block_Statements[0].get(), "2", 0);
				});

				test_block("consecutive if statements", "if (1) {} if (2) {} int y = 2;", [&](const Program& program){
						return program.statements.size() == 3 &&
								is_if(program.statements[0].get(), "1", 0) &&
								is_if(program.statements[1].get(), "2", 0) &&
								is_declaration(program.statements[2].get(), "y", 2);
				});

				test_block("consecutive functions", "int f() {} int g() {} int y = 2;", [&](const Program& program){
						return program.statements.size() == 3 &&
								is_function(program.statements[0].get(), "f", 0) &&
								is_function(program.statements[1].get(), "g", 0) &&
								is_declaration(program.statements[2].get(), "y", 2);
				});

				assert(all_blocks_preserved && "Block statements must preserve subsequent statements and enclosing scope");
		}

		{
				Parser parser(lexxer{"f(1);"});
				auto program = parser.ParseProgram();
				assert(program->statements.size() == 1);
				auto statement = dynamic_cast<ExpressionStatement*>(program->statements[0].get());
				assert(statement && statement->expr);
				auto call = dynamic_cast<CallExpression*>(statement->expr.get());
				assert(call && call->arguments);
				assert(call->function_name && call->function_name->TokenLiteral() == "f");
				assert(call->arguments->size() == 1);
				assert(call->arguments->at(0));
				assert(call->arguments->at(0)->TokenLiteral() == "1");
		}

//		std::string s;

		//Parse_input(std::cin,s);


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
