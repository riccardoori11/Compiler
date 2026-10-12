#include "evaluator.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

// Exercise the unsupported-node path without depending on a language feature
// that may be implemented later.
class UnsupportedExpression final : public Expression {
public:
    std::string TokenLiteral() const override { return "unsupported"; }
    void print() const override {}
};

Value evaluate_for_test(Evaluator& evaluator, const Node& node) {
    return evaluator.eval(node);
}

Value evaluate_for_test(Evaluator& evaluator, const VectorStatement& statements) {
    return evaluator.evalStatement(statements);
}

std::unique_ptr<Statement> make_integer_statement(int value) {
    const Token token{TokenType::INTEGER, std::to_string(value)};
    auto statement = std::make_unique<ExpressionStatement>(token);
    statement->expr = std::make_unique<Integer_Liter>(token, value);
    return statement;
}

std::unique_ptr<Statement> make_boolean_statement(bool value) {
    const Token token{TokenType::IDENTIFIER, value ? "true" : "false"};
    auto statement = std::make_unique<ExpressionStatement>(token);
    statement->expr = std::make_unique<Boolean>(token, value);
    return statement;
}

std::unique_ptr<Statement> make_unsupported_statement() {
    auto statement = std::make_unique<ExpressionStatement>(Token{TokenType::ILLEGAL, "unsupported"});
    statement->expr = std::make_unique<UnsupportedExpression>();
    return statement;
}

} // namespace

int main() {
    Evaluator evaluator;
    int checks = 0;
    int failures = 0;

    auto expect_value = [&](const char* name, const auto& input, const Value& expected) {
        ++checks;
        try {
            const Value actual = evaluate_for_test(evaluator, input);
            // Variant equality checks the held type as well as the value.
            if (actual == expected) {
                std::cout << "PASS: " << name << '\n';
                return;
            }
            std::cerr << "FAIL: " << name << ": unexpected value or runtime type\n";
        } catch (const std::exception& error) {
            std::cerr << "FAIL: " << name << ": " << error.what() << '\n';
        }
        ++failures;
    };

    auto expect_runtime_error = [&](const char* name, const auto& input) {
        ++checks;
        try {
            evaluate_for_test(evaluator, input);
            std::cerr << "FAIL: " << name << ": expected std::runtime_error\n";
        } catch (const std::runtime_error&) {
            std::cout << "PASS: " << name << '\n';
            return;
        } catch (const std::exception& error) {
            std::cerr << "FAIL: " << name << ": unexpected exception: " << error.what() << '\n';
        }
        ++failures;
    };

    const Integer_Liter positive{Token{TokenType::INTEGER, "42"}, 42};
    const Integer_Liter zero{Token{TokenType::INTEGER, "0"}, 0};
    const Integer_Liter negative{Token{TokenType::INTEGER, "-7"}, -7};
    expect_value("integer literal returns 42", positive, Value{42});
    expect_value("zero remains an integer", zero, Value{0});
    expect_value("negative stored integer value", negative, Value{-7});

    // Boolean keyword tokens/parsing are not implemented yet. Construct the
    // existing Boolean nodes directly to test evaluation independently.
    const Boolean true_literal{Token{TokenType::IDENTIFIER, "true"}, true};
    const Boolean false_literal{Token{TokenType::IDENTIFIER, "false"}, false};
    expect_value("true remains a boolean", true_literal, Value{true});
    expect_value("false remains a boolean", false_literal, Value{false});

    ExpressionStatement integer_statement{Token{TokenType::INTEGER, "8"}};
    integer_statement.expr = std::make_unique<Integer_Liter>(Token{TokenType::INTEGER, "8"}, 8);
    expect_value("expression statement returns its integer value", integer_statement, Value{8});

    ExpressionStatement boolean_statement{Token{TokenType::IDENTIFIER, "false"}};
    boolean_statement.expr = std::make_unique<Boolean>(Token{TokenType::IDENTIFIER, "false"}, false);
    expect_value("expression statement preserves boolean type", boolean_statement, Value{false});

    const UnsupportedExpression unsupported;
    expect_runtime_error("unsupported AST node is rejected", unsupported);

    ExpressionStatement unsupported_statement{Token{TokenType::ILLEGAL, "unsupported"}};
    unsupported_statement.expr = std::make_unique<UnsupportedExpression>();
    expect_runtime_error("expression statement propagates evaluation errors", unsupported_statement);

    // These checks call evalStatement directly with its actual container type.
    const VectorStatement empty_statements;
    expect_runtime_error("empty statement list is rejected", empty_statements);

    VectorStatement single_statement;
    single_statement.push_back(make_integer_statement(42));
    expect_value("single statement returns its value", single_statement, Value{42});

    VectorStatement integer_statements;
    integer_statements.push_back(make_integer_statement(2));
    integer_statements.push_back(make_integer_statement(7));
    integer_statements.push_back(make_integer_statement(42));
    expect_value("statement list returns the last value", integer_statements, Value{42});

    VectorStatement mixed_statements;
    mixed_statements.push_back(make_integer_statement(1));
    mixed_statements.push_back(make_boolean_statement(false));
    expect_value("statement list preserves final boolean type", mixed_statements, Value{false});
    mixed_statements.push_back(make_integer_statement(1));
    expect_value("statement list switches back to integer type", mixed_statements, Value{1});

    VectorStatement first_statement_error;
    first_statement_error.push_back(make_unsupported_statement());
    first_statement_error.push_back(make_integer_statement(42));
    expect_runtime_error("first statement error is not skipped", first_statement_error);

    VectorStatement middle_statement_error;
    middle_statement_error.push_back(make_integer_statement(2));
    middle_statement_error.push_back(make_unsupported_statement());
    middle_statement_error.push_back(make_integer_statement(42));
    expect_runtime_error("middle statement error propagates instead of returning the last value", middle_statement_error);

    Program program;
    program.statements = std::move(integer_statements);
    expect_value("program evaluation returns the last statement value", program, Value{42});

    const Program empty_program;
    expect_runtime_error("program evaluation rejects an empty statement list", empty_program);

    std::cout << checks - failures << '/' << checks << " evaluator tests passed\n";
    return failures == 0 ? 0 : 1;
}
