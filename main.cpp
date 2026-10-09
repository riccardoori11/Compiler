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

		// Regression tests for error.md #4: calls with one argument must
		// retain that argument, consume ')', and preserve the next statement.
		{
				auto is_integer = [](const Expression* expression, int value){
						auto integer = dynamic_cast<const Integer_Liter*>(expression);
						return integer && integer->value == value;
				};
				auto is_identifier = [](const Expression* expression, const char* name){
						auto identifier = dynamic_cast<const Identifier*>(expression);
						return identifier && identifier->TokenLiteral() == name;
				};
				auto get_call = [&](const Expression* expression, const char* name,
								std::size_t argument_count) -> const CallExpression* {
						auto call = dynamic_cast<const CallExpression*>(expression);
						if (!call || !is_identifier(call->function_name.get(), name) || !call->arguments ||
								call->arguments->size() != argument_count){
								return nullptr;
						}
						for (const auto& argument : *call->arguments){
								if (!argument){
										return nullptr;
								}
						}
						return call;
				};
				auto get_statement_call = [&](const Statement* statement, const char* name,
								std::size_t argument_count) -> const CallExpression* {
						auto expression_statement = dynamic_cast<const ExpressionStatement*>(statement);
						return expression_statement ? get_call(expression_statement->expr.get(), name, argument_count)
								: nullptr;
				};

				bool all_calls_correct = true;
				auto test_call = [&](const char* name, const char* input, auto check){
						try {
								Parser parser(lexxer{input});
								auto program = parser.ParseProgram();
								if (program && check(*program)){
										return;
								}
								std::cerr << "FAIL: error.md #4 (" << name << "): unexpected AST: " << input << '\n';
						} catch (const std::runtime_error& error) {
								std::cerr << "FAIL: error.md #4 (" << name << "): " << error.what()
										<< ": " << input << '\n';
						}
						all_calls_correct = false;
				};

				struct IntegerCallCase {
						const char* name;
						const char* input;
						std::size_t argument_count;
				};
				const IntegerCallCase integer_calls[] = {
						{"one integer argument", "f(1);", 1},
						{"zero arguments", "f();", 0},
						{"two arguments", "f(1, 2);", 2},
						{"three arguments", "f(1, 2, 3);", 3}
				};
				for (const auto& test_case : integer_calls){
						test_call(test_case.name, test_case.input, [&](const Program& program){
								if (program.statements.size() != 1){
										return false;
								}
								auto call = get_statement_call(program.statements[0].get(), "f", test_case.argument_count);
								if (!call){
										return false;
								}
								for (std::size_t i{}; i < test_case.argument_count; ++i){
										if (!is_integer(call->arguments->at(i).get(), static_cast<int>(i) + 1)){
												return false;
										}
								}
								return true;
						});
				}

				test_call("one identifier argument", "f(x);", [&](const Program& program){
						auto call = program.statements.size() == 1 ? get_statement_call(program.statements[0].get(), "f", 1) : nullptr;
						return call && is_identifier(call->arguments->at(0).get(), "x");
				});

				test_call("one prefix argument", "f(-1);", [&](const Program& program){
						auto call = program.statements.size() == 1 ? get_statement_call(program.statements[0].get(), "f", 1) : nullptr;
						auto prefix = call ? dynamic_cast<const PrefixExpression*>(call->arguments->at(0).get()) : nullptr;
						return prefix && prefix->TokenLiteral() == "-" && is_integer(prefix->right.get(), 1);
				});

				test_call("one infix argument preserves precedence", "f(1 + 2 * 3);", [&](const Program& program){
						auto call = program.statements.size() == 1 ? get_statement_call(program.statements[0].get(), "f", 1) : nullptr;
						auto sum = call ? dynamic_cast<const InfixExpression*>(call->arguments->at(0).get()) : nullptr;
						auto product = sum ? dynamic_cast<const InfixExpression*>(sum->right.get()) : nullptr;
						return sum && sum->TokenLiteral() == "+" && is_integer(sum->left.get(), 1) &&
								product && product->TokenLiteral() == "*" && is_integer(product->left.get(), 2) &&
								is_integer(product->right.get(), 3);
				});

				test_call("one grouped argument", "f((1 + 2));", [&](const Program& program){
						auto call = program.statements.size() == 1 ? get_statement_call(program.statements[0].get(), "f", 1) : nullptr;
						auto sum = call ? dynamic_cast<const InfixExpression*>(call->arguments->at(0).get()) : nullptr;
						return sum && sum->TokenLiteral() == "+" && is_integer(sum->left.get(), 1) &&
								is_integer(sum->right.get(), 2);
				});

				test_call("nested calls with one argument", "f(g(1));", [&](const Program& program){
						auto outer = program.statements.size() == 1 ? get_statement_call(program.statements[0].get(), "f", 1) : nullptr;
						auto inner = outer ? get_call(outer->arguments->at(0).get(), "g", 1) : nullptr;
						return inner && is_integer(inner->arguments->at(0).get(), 1);
				});

				test_call("call in a variable initializer", "int x = f(1);", [&](const Program& program){
						auto declaration = program.statements.size() == 1 ? dynamic_cast<const VariableDeclaration*>(program.statements[0].get()) : nullptr;
						auto call = declaration ? get_call(declaration->value.get(), "f", 1) : nullptr;
						return declaration && declaration->TokenLiteral() == "int" && declaration->name &&
								declaration->name->TokenLiteral() == "x" && call && is_integer(call->arguments->at(0).get(), 1);
				});

				test_call("call in a return statement", "return f(1);", [&](const Program& program){
						auto return_statement = program.statements.size() == 1 ? dynamic_cast<const Return*>(program.statements[0].get()) : nullptr;
						auto call = return_statement ? get_call(return_statement->Returnvalue.get(), "f", 1) : nullptr;
						return call && is_integer(call->arguments->at(0).get(), 1);
				});

				test_call("infix expression after a call", "f(1) + 2;", [&](const Program& program){
						auto statement = program.statements.size() == 1 ? dynamic_cast<const ExpressionStatement*>(program.statements[0].get()) : nullptr;
						auto sum = statement ? dynamic_cast<const InfixExpression*>(statement->expr.get()) : nullptr;
						auto call = sum ? get_call(sum->left.get(), "f", 1) : nullptr;
						return sum && sum->TokenLiteral() == "+" && call && is_integer(call->arguments->at(0).get(), 1) &&
								is_integer(sum->right.get(), 2);
				});

				test_call("declaration after a call", "f(1); int y = 2;", [&](const Program& program){
						if (program.statements.size() != 2){
								return false;
						}
						auto call = get_statement_call(program.statements[0].get(), "f", 1);
						auto declaration = dynamic_cast<const VariableDeclaration*>(program.statements[1].get());
						return call && is_integer(call->arguments->at(0).get(), 1) && declaration &&
								declaration->TokenLiteral() == "int" && declaration->name &&
								declaration->name->TokenLiteral() == "y" && is_integer(declaration->value.get(), 2);
				});

				test_call("consecutive calls", "f(1); g(2);", [&](const Program& program){
						if (program.statements.size() != 2){
								return false;
						}
						auto first = get_statement_call(program.statements[0].get(), "f", 1);
						auto second = get_statement_call(program.statements[1].get(), "g", 1);
						return first && is_integer(first->arguments->at(0).get(), 1) &&
								second && is_integer(second->arguments->at(0).get(), 2);
				});

				const char* invalid_calls[] = {
						"f(1;",
						"f(,);",
						"f(1,);",
						"f(1 2);",
						"f(1, @);"
				};
				for (const char* input : invalid_calls){
						bool rejected = false;
						try {
								Parser parser(lexxer{input});
								auto program = parser.ParseProgram();
								rejected = program && program->statements.empty();
						} catch (const std::runtime_error&) {
								rejected = true;
						}
						if (!rejected){
								std::cerr << "FAIL: error.md #4: malformed call returned AST statements: " << input << '\n';
								all_calls_correct = false;
						}
				}

				assert(all_calls_correct && "Calls must preserve their arguments and following statements, and reject malformed argument lists");
		}

		// Regression tests for error.md #5: braces and 'else' must not
		// become extra statements, and genuine block statements must remain intact.
		{
				auto is_integer = [](const Expression* expression, int value){
						auto integer = dynamic_cast<const Integer_Liter*>(expression);
						return integer && integer->value == value;
				};
				auto is_declaration = [&](const Statement* statement, const char* name, int value){
						auto declaration = dynamic_cast<const VariableDeclaration*>(statement);
						return declaration && declaration->TokenLiteral() == "int" && declaration->name &&
								declaration->name->TokenLiteral() == name && is_integer(declaration->value.get(), value);
				};
				auto is_return = [](const Statement* statement, const char* value){
						auto return_statement = dynamic_cast<const Return*>(statement);
						return return_statement && return_statement->Returnvalue &&
								return_statement->Returnvalue->TokenLiteral() == value;
				};
				auto get_if = [&](const Statement* statement, int condition) -> const IfStatement* {
						auto conditional = dynamic_cast<const IfStatement*>(statement);
						return conditional && is_integer(conditional->condition.get(), condition) && conditional->Consequence
								? conditional : nullptr;
				};
				auto get_function = [](const Program& program) -> const FunctionLiteral* {
						if (program.statements.size() != 1){
								return nullptr;
						}
						auto function = dynamic_cast<const FunctionLiteral*>(program.statements[0].get());
						return function && function->name && function->name->TokenLiteral() == "f" &&
								function->Parameters.empty() && function->FunctionBody ? function : nullptr;
				};

				bool all_block_contents_correct = true;
				auto test_block_contents = [&](const char* name, const char* input, auto check){
						try {
								Parser parser(lexxer{input});
								auto program = parser.ParseProgram();
								if (program && check(*program)){
										return;
								}
								std::cerr << "FAIL: error.md #5 (" << name << "): unexpected block contents: " << input << '\n';
						} catch (const std::runtime_error& error) {
								std::cerr << "FAIL: error.md #5 (" << name << "): " << error.what()
										<< ": " << input << '\n';
						}
						all_block_contents_correct = false;
				};

				struct ConditionalBlockCase {
						const char* name;
						const char* input;
						std::size_t consequence_size;
						bool has_else;
						std::size_t alternative_size;
				};
				const ConditionalBlockCase conditional_blocks[] = {
						{"empty consequence", "if (1) {}", 0, false, 0},
						{"empty consequence and alternative", "if (1) {} else {}", 0, true, 0},
						{"one consequence declaration", "if (1) { int x = 1; }", 1, false, 0},
						{"one declaration in each branch", "if (1) { int x = 1; } else { int x = 3; }", 1, true, 1},
						{"empty consequence with populated alternative", "if (1) {} else { int x = 3; }", 0, true, 1},
						{"populated consequence with empty alternative", "if (1) { int x = 1; } else {}", 1, true, 0},
						{"multiple declarations in each branch", "if (1) { int x = 1; int y = 2; } else { int x = 3; int y = 4; }", 2, true, 2}
				};
				const char* declaration_names[] = {"x", "y"};
				for (const auto& test_case : conditional_blocks){
						test_block_contents(test_case.name, test_case.input, [&](const Program& program){
								auto conditional = program.statements.size() == 1 ? get_if(program.statements[0].get(), 1) : nullptr;
								if (!conditional || conditional->Consequence->Block_Statements.size() != test_case.consequence_size){
										return false;
								}
								for (std::size_t i{}; i < test_case.consequence_size; ++i){
										if (!is_declaration(conditional->Consequence->Block_Statements[i].get(), declaration_names[i], static_cast<int>(i) + 1)){
												return false;
										}
								}
								if (!test_case.has_else){
										return conditional->Alternative == nullptr;
								}
								if (!conditional->Alternative || conditional->Alternative->Block_Statements.size() != test_case.alternative_size){
										return false;
								}
								for (std::size_t i{}; i < test_case.alternative_size; ++i){
										if (!is_declaration(conditional->Alternative->Block_Statements[i].get(), declaration_names[i], static_cast<int>(i) + 3)){
												return false;
										}
								}
								return true;
						});
				}

				test_block_contents("empty function body", "int f() {}", [&](const Program& program){
						auto function = get_function(program);
						return function && function->FunctionBody->Block_Statements.empty();
				});

				test_block_contents("one return in function body", "int f() { return 1; }", [&](const Program& program){
						auto function = get_function(program);
						return function && function->FunctionBody->Block_Statements.size() == 1 &&
								is_return(function->FunctionBody->Block_Statements[0].get(), "1");
				});

				test_block_contents("declaration and return in function body", "int f() { int x = 1; return x; }", [&](const Program& program){
						auto function = get_function(program);
						return function && function->FunctionBody->Block_Statements.size() == 2 &&
								is_declaration(function->FunctionBody->Block_Statements[0].get(), "x", 1) &&
								is_return(function->FunctionBody->Block_Statements[1].get(), "x");
				});

				test_block_contents("real expression statements in both branches", "if (1) { f(1); } else { 2 + 3; }", [&](const Program& program){
						auto conditional = program.statements.size() == 1 ? get_if(program.statements[0].get(), 1) : nullptr;
						if (!conditional || conditional->Consequence->Block_Statements.size() != 1 ||
								!conditional->Alternative || conditional->Alternative->Block_Statements.size() != 1){
								return false;
						}
						auto consequence = dynamic_cast<const ExpressionStatement*>(conditional->Consequence->Block_Statements[0].get());
						auto call = consequence ? dynamic_cast<const CallExpression*>(consequence->expr.get()) : nullptr;
						auto callee = call ? dynamic_cast<const Identifier*>(call->function_name.get()) : nullptr;
						auto alternative = dynamic_cast<const ExpressionStatement*>(conditional->Alternative->Block_Statements[0].get());
						auto sum = alternative ? dynamic_cast<const InfixExpression*>(alternative->expr.get()) : nullptr;
						return callee && callee->TokenLiteral() == "f" && call->arguments && call->arguments->size() == 1 &&
								is_integer(call->arguments->at(0).get(), 1) && sum && sum->TokenLiteral() == "+" &&
								is_integer(sum->left.get(), 2) && is_integer(sum->right.get(), 3);
				});

				test_block_contents("nested empty branches", "if (1) { if (2) {} else {} } else { if (3) {} else {} }", [&](const Program& program){
						auto outer = program.statements.size() == 1 ? get_if(program.statements[0].get(), 1) : nullptr;
						if (!outer || outer->Consequence->Block_Statements.size() != 1 ||
								!outer->Alternative || outer->Alternative->Block_Statements.size() != 1){
								return false;
						}
						auto consequence = get_if(outer->Consequence->Block_Statements[0].get(), 2);
						auto alternative = get_if(outer->Alternative->Block_Statements[0].get(), 3);
						return consequence && consequence->Consequence->Block_Statements.empty() && consequence->Alternative &&
								consequence->Alternative->Block_Statements.empty() && alternative &&
								alternative->Consequence->Block_Statements.empty() && alternative->Alternative &&
								alternative->Alternative->Block_Statements.empty();
				});

				test_block_contents("populated branches inside a function", "int f() { if (1) { int x = 2; } else { int y = 3; } return 4; }", [&](const Program& program){
						auto function = get_function(program);
						if (!function || function->FunctionBody->Block_Statements.size() != 2){
								return false;
						}
						const auto& body = function->FunctionBody->Block_Statements;
						auto conditional = get_if(body[0].get(), 1);
						return conditional && conditional->Consequence->Block_Statements.size() == 1 &&
								conditional->Alternative && conditional->Alternative->Block_Statements.size() == 1 &&
								is_declaration(conditional->Consequence->Block_Statements[0].get(), "x", 2) &&
								is_declaration(conditional->Alternative->Block_Statements[0].get(), "y", 3) && is_return(body[1].get(), "4");
				});

				assert(all_block_contents_correct && "Blocks must contain only their actual statements, never braces or else tokens");
		}

		// Regression tests for error.md #6: EOF must not stand in for '}'.
		// Each unfinished input is paired with a correctly closed version.
		{
				struct BlockTerminationCase {
						const char* name;
						const char* unfinished_input;
						const char* closed_input;
						std::size_t statement_count;
				};
				const BlockTerminationCase block_terminations[] = {
						{"empty consequence", "if (1) {", "if (1) {}", 1},
						{"populated consequence", "if (1) { int x = 2;", "if (1) { int x = 2; }", 1},
						{"empty alternative", "if (1) {} else {", "if (1) {} else {}", 1},
						{"populated alternative", "if (1) {} else { int x = 2;", "if (1) {} else { int x = 2; }", 1},
						{"empty function body", "int f() {", "int f() {}", 1},
						{"populated function body", "int f() { return 1;", "int f() { return 1; }", 1},
						{"missing outer if brace", "if (1) { if (2) {}", "if (1) { if (2) {} }", 1},
						{"missing inner and outer if braces", "if (1) { if (2) {", "if (1) { if (2) {} }", 1},
						{"missing function brace after nested if", "int f() { if (1) {}", "int f() { if (1) {} }", 1},
						{"missing function brace after nested if/else", "int f() { if (1) {} else {}", "int f() { if (1) {} else {} }", 1},
						{"valid declaration before unfinished block", "int x = 0; if (1) {", "int x = 0; if (1) {}", 2}
				};

				bool all_blocks_terminated = true;
				for (const auto& test_case : block_terminations){
						bool rejected = false;
						try {
								Parser parser(lexxer{test_case.unfinished_input});
								auto program = parser.ParseProgram();
								rejected = !program || program->statements.empty();
						} catch (const std::runtime_error&) {
								rejected = true;
						}
						if (!rejected){
								std::cerr << "FAIL: error.md #6 (" << test_case.name << "): unfinished block returned AST statements: "
										<< test_case.unfinished_input << '\n';
								all_blocks_terminated = false;
						}

						// A real closing brace at EOF must still be accepted.
						try {
								Parser parser(lexxer{test_case.closed_input});
								auto program = parser.ParseProgram();
								bool accepted = program && program->statements.size() == test_case.statement_count;
								if (accepted){
										for (const auto& statement : program->statements){
												if (!statement){
														accepted = false;
														break;
												}
										}
								}
								if (!accepted){
										std::cerr << "FAIL: error.md #6 (" << test_case.name << "): closed block returned an unexpected AST: "
												<< test_case.closed_input << '\n';
										all_blocks_terminated = false;
								}
						} catch (const std::runtime_error& error) {
								std::cerr << "FAIL: error.md #6 (" << test_case.name << "): closed block was rejected: "
										<< error.what() << ": " << test_case.closed_input << '\n';
								all_blocks_terminated = false;
						}
				}

				assert(all_blocks_terminated && "Unfinished blocks must be rejected, and properly closed blocks must be accepted");
		}

		// Regression tests for error.md #7: an omitted ';' may be rejected
		// or allowed, but it must never discard tokens or change statements/scope.
		{
				auto is_integer = [](const Expression* expression, int value){
						auto integer = dynamic_cast<const Integer_Liter*>(expression);
						return integer && integer->value == value;
				};
				auto is_declaration = [&](const Statement* statement, const char* name, int value){
						auto declaration = dynamic_cast<const VariableDeclaration*>(statement);
						return declaration && declaration->TokenLiteral() == "int" && declaration->name &&
								declaration->name->TokenLiteral() == name && is_integer(declaration->value.get(), value);
				};
				auto is_return = [&](const Statement* statement, int value){
						auto return_statement = dynamic_cast<const Return*>(statement);
						return return_statement && is_integer(return_statement->Returnvalue.get(), value);
				};
				auto is_call = [&](const Statement* statement, const char* name, int argument){
						auto expression_statement = dynamic_cast<const ExpressionStatement*>(statement);
						auto call = expression_statement ? dynamic_cast<const CallExpression*>(expression_statement->expr.get()) : nullptr;
						auto callee = call ? dynamic_cast<const Identifier*>(call->function_name.get()) : nullptr;
						return callee && callee->TokenLiteral() == name && call->arguments && call->arguments->size() == 1 &&
								is_integer(call->arguments->at(0).get(), argument);
				};
				auto get_function = [](const Statement* statement) -> const FunctionLiteral* {
						auto function = dynamic_cast<const FunctionLiteral*>(statement);
						return function && function->name && function->name->TokenLiteral() == "f" &&
								function->Parameters.empty() && function->FunctionBody ? function : nullptr;
				};
				auto get_if = [&](const Statement* statement) -> const IfStatement* {
						auto conditional = dynamic_cast<const IfStatement*>(statement);
						return conditional && is_integer(conditional->condition.get(), 1) && conditional->Consequence
								? conditional : nullptr;
				};

				bool all_terminators_correct = true;
				auto test_terminator = [&](const char* name, const char* missing_semicolon,
								const char* with_semicolon, auto check){
						try {
								Parser parser(lexxer{missing_semicolon});
								auto program = parser.ParseProgram();
								bool rejected = !program || program->statements.empty();
								if (!rejected && !check(*program)){
										std::cerr << "FAIL: error.md #7 (" << name << "): omitted semicolon changed the AST: "
												<< missing_semicolon << '\n';
										all_terminators_correct = false;
								}
						} catch (const std::runtime_error&) {
								// Rejection is valid when semicolons are required.
						}

						try {
								Parser parser(lexxer{with_semicolon});
								auto program = parser.ParseProgram();
								if (!program || !check(*program)){
										std::cerr << "FAIL: error.md #7 (" << name << "): terminated statements returned an unexpected AST: "
												<< with_semicolon << '\n';
										all_terminators_correct = false;
								}
						} catch (const std::runtime_error& error) {
								std::cerr << "FAIL: error.md #7 (" << name << "): terminated statements were rejected: "
										<< error.what() << ": " << with_semicolon << '\n';
								all_terminators_correct = false;
						}
				};

				test_terminator("consecutive declarations", "int x = 1 int y = 2;", "int x = 1; int y = 2;", [&](const Program& program){
						return program.statements.size() == 2 && is_declaration(program.statements[0].get(), "x", 1) &&
								is_declaration(program.statements[1].get(), "y", 2);
				});

				test_terminator("consecutive returns", "return 1 return 2;", "return 1; return 2;", [&](const Program& program){
						return program.statements.size() == 2 && is_return(program.statements[0].get(), 1) &&
								is_return(program.statements[1].get(), 2);
				});

				test_terminator("return after declaration", "int x = 1 return 2;", "int x = 1; return 2;", [&](const Program& program){
						return program.statements.size() == 2 && is_declaration(program.statements[0].get(), "x", 1) &&
								is_return(program.statements[1].get(), 2);
				});

				test_terminator("declaration after return", "return 1 int x = 2;", "return 1; int x = 2;", [&](const Program& program){
						return program.statements.size() == 2 && is_return(program.statements[0].get(), 1) &&
								is_declaration(program.statements[1].get(), "x", 2);
				});

				test_terminator("call after declaration", "int x = 1 g(2);", "int x = 1; g(2);", [&](const Program& program){
						return program.statements.size() == 2 && is_declaration(program.statements[0].get(), "x", 1) &&
								is_call(program.statements[1].get(), "g", 2);
				});

				test_terminator("call after return", "return 1 g(2);", "return 1; g(2);", [&](const Program& program){
						return program.statements.size() == 2 && is_return(program.statements[0].get(), 1) &&
								is_call(program.statements[1].get(), "g", 2);
				});

				test_terminator("declaration at EOF", "int x = 1", "int x = 1;", [&](const Program& program){
						return program.statements.size() == 1 && is_declaration(program.statements[0].get(), "x", 1);
				});

				test_terminator("return at EOF", "return 1", "return 1;", [&](const Program& program){
						return program.statements.size() == 1 && is_return(program.statements[0].get(), 1);
				});

				test_terminator("declaration before function closing brace", "int f() { int x = 1 } int y = 2;", "int f() { int x = 1; } int y = 2;", [&](const Program& program){
						if (program.statements.size() != 2 || !is_declaration(program.statements[1].get(), "y", 2)){
								return false;
						}
						auto function = get_function(program.statements[0].get());
						return function && function->FunctionBody->Block_Statements.size() == 1 &&
								is_declaration(function->FunctionBody->Block_Statements[0].get(), "x", 1);
				});

				test_terminator("return before function closing brace", "int f() { return 1 } int y = 2;", "int f() { return 1; } int y = 2;", [&](const Program& program){
						if (program.statements.size() != 2 || !is_declaration(program.statements[1].get(), "y", 2)){
								return false;
						}
						auto function = get_function(program.statements[0].get());
						return function && function->FunctionBody->Block_Statements.size() == 1 &&
								is_return(function->FunctionBody->Block_Statements[0].get(), 1);
				});

				test_terminator("consecutive returns inside function", "int f() { return 1 return 2; }", "int f() { return 1; return 2; }", [&](const Program& program){
						auto function = program.statements.size() == 1 ? get_function(program.statements[0].get()) : nullptr;
						return function && function->FunctionBody->Block_Statements.size() == 2 &&
								is_return(function->FunctionBody->Block_Statements[0].get(), 1) &&
								is_return(function->FunctionBody->Block_Statements[1].get(), 2);
				});

				test_terminator("return after declaration inside function", "int f() { int x = 1 return 2; }", "int f() { int x = 1; return 2; }", [&](const Program& program){
						auto function = program.statements.size() == 1 ? get_function(program.statements[0].get()) : nullptr;
						return function && function->FunctionBody->Block_Statements.size() == 2 &&
								is_declaration(function->FunctionBody->Block_Statements[0].get(), "x", 1) &&
								is_return(function->FunctionBody->Block_Statements[1].get(), 2);
				});

				test_terminator("consecutive returns inside if", "if (1) { return 1 return 2; }", "if (1) { return 1; return 2; }", [&](const Program& program){
						auto conditional = program.statements.size() == 1 ? get_if(program.statements[0].get()) : nullptr;
						return conditional && !conditional->Alternative && conditional->Consequence->Block_Statements.size() == 2 &&
								is_return(conditional->Consequence->Block_Statements[0].get(), 1) &&
								is_return(conditional->Consequence->Block_Statements[1].get(), 2);
				});

				test_terminator("call after declaration inside else", "if (1) {} else { int x = 1 g(2); }", "if (1) {} else { int x = 1; g(2); }", [&](const Program& program){
						auto conditional = program.statements.size() == 1 ? get_if(program.statements[0].get()) : nullptr;
						return conditional && conditional->Consequence->Block_Statements.empty() && conditional->Alternative &&
								conditional->Alternative->Block_Statements.size() == 2 &&
								is_declaration(conditional->Alternative->Block_Statements[0].get(), "x", 1) &&
								is_call(conditional->Alternative->Block_Statements[1].get(), "g", 2);
				});

				assert(all_terminators_correct && "Declaration and return terminators must not consume unrelated tokens or change scope");
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
