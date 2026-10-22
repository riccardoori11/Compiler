#include "evaluator.hpp"
#include "Environment.hpp"
#include "parser.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace {

bool same_value(const Value& actual, const Value& expected) {
    return std::visit([&](auto expected_value) {
        using T = decltype(expected_value);
        const auto* actual_value = std::get_if<T>(&actual);
        if (!actual_value) {
            return false;
        }
        if constexpr (std::is_floating_point_v<T>) {
            // Allow rounding differences while still checking the variant type.
            const T tolerance = T{4} * std::numeric_limits<T>::epsilon()
                * std::max(T{1}, std::abs(expected_value));
            return std::abs(*actual_value - expected_value) <= tolerance;
        } else {
            return *actual_value == expected_value;
        }
    }, expected);
}

std::string describe(const Value& value) {
    return std::visit([](auto item) -> std::string {
        using T = decltype(item);
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "null";
        } else if constexpr (std::is_same_v<T, bool>) {
            return item ? "bool(true)" : "bool(false)";
        } else if constexpr (std::is_same_v<T, int>) {
            return "int(" + std::to_string(item) + ")";
        } else if constexpr (std::is_same_v<T, float>) {
            return "float(" + std::to_string(item) + ")";
        } else if constexpr (std::is_same_v<T, double>) {
            return "double(" + std::to_string(item) + ")";
        } else {
            return item ? "function" : "function(null)";
        }
    }, value);
}

class Checks {
    int checks = 0;
    int failures = 0;

public:
    template <typename Evaluate>
    void expect_value(std::string_view name, Evaluate evaluate, const Value& expected) {
        ++checks;
        try {
            const Value actual = evaluate();
            if (same_value(actual, expected)) {
                return;
            }
            std::cerr << "FAIL: " << name << ": expected " << describe(expected)
                      << ", got " << describe(actual) << '\n';
        } catch (const std::exception& error) {
            std::cerr << "FAIL: " << name << ": unexpected exception: "
                      << error.what() << '\n';
        }
        ++failures;
    }

    template <typename Evaluate>
    void expect_runtime_error(std::string_view name, Evaluate evaluate,
                              std::string_view expected_message = {}) {
        ++checks;
        try {
            evaluate();
            std::cerr << "FAIL: " << name << ": expected std::runtime_error\n";
        } catch (const std::runtime_error& error) {
            if (expected_message.empty()
                || std::string_view{error.what()}.find(expected_message) != std::string_view::npos) {
                return;
            }
            std::cerr << "FAIL: " << name << ": expected error containing \""
                      << expected_message << "\", got \"" << error.what() << "\"\n";
        } catch (const std::exception& error) {
            std::cerr << "FAIL: " << name << ": unexpected exception: "
                      << error.what() << '\n';
        }
        ++failures;
    }

    int finish() const {
        std::cout << checks - failures << '/' << checks
                  << " evaluator operator checks passed\n";
        return failures == 0 ? 0 : 1;
    }
};

std::unique_ptr<Expression> integer(int value) {
    return std::make_unique<Integer_Liter>(Token{TokenType::INTEGER, ""}, value);
}

std::unique_ptr<Expression> floating(float value) {
    return std::make_unique<FloatLiter>(Token{TokenType::FLOAT, ""}, value);
}

std::unique_ptr<Expression> boolean(bool value) {
    return std::make_unique<Boolean>(
        Token{value ? TokenType::TRUE : TokenType::FALSE, ""}, value);
}

std::unique_ptr<Expression> prefix(TokenType op, std::unique_ptr<Expression> right) {
    return std::make_unique<PrefixExpression>(Token{op, ""}, std::move(right));
}

std::unique_ptr<Expression> infix(TokenType op, std::unique_ptr<Expression> left,
                                  std::unique_ptr<Expression> right) {
    return std::make_unique<InfixExpression>(
        Token{op, ""}, std::move(left), std::move(right));
}

std::unique_ptr<ExpressionStatement> statement(std::unique_ptr<Expression> expression) {
    auto result = std::make_unique<ExpressionStatement>(Token{TokenType::SEMICOLON, ";"});
    result->expr = std::move(expression);
    return result;
}

std::unique_ptr<Return> return_statement(std::unique_ptr<Expression> expression) {
    auto result = std::make_unique<Return>(Token{TokenType::RETURN, "return"});
    result->Returnvalue = std::move(expression);
    return result;
}

template <typename... Statements>
std::unique_ptr<BlockStatements> block(Statements... statements) {
    auto result = std::make_unique<BlockStatements>();
    (result->Block_Statements.push_back(std::move(statements)), ...);
    return result;
}

std::unique_ptr<IfStatement> conditional(std::unique_ptr<Expression> condition,
                                         std::unique_ptr<BlockStatements> consequence,
                                         std::unique_ptr<BlockStatements> alternative = nullptr) {
    auto result = std::make_unique<IfStatement>(Token{TokenType::IF, "if"});
    result->condition = std::move(condition);
    result->Consequence = std::move(consequence);
    result->Alternative = std::move(alternative);
    return result;
}

Value execute_ordinary_statement(Evaluator& evaluator, const Statement& node,
                                 Environment& environment) {
    const ExecutionResult result = evaluator.execute(node, environment);
    if (result.is_return) {
        throw std::logic_error("Ordinary statement unexpectedly produced a return signal");
    }
    return result.value;
}

void check_integer_arithmetic(Evaluator& evaluator, Environment& environment, Checks& checks) {
    constexpr int minimum = std::numeric_limits<int>::min();
    constexpr int maximum = std::numeric_limits<int>::max();

    struct IntegerCase {
        const char* name;
        TokenType op;
        int left;
        int right;
        int expected;
    };
    const IntegerCase valid_cases[] = {
        {"addition reaches maximum", TokenType::PLUS, maximum - 1, 1, maximum},
        {"addition reaches minimum", TokenType::PLUS, minimum + 1, -1, minimum},
        {"maximum plus zero", TokenType::PLUS, maximum, 0, maximum},
        {"minimum plus zero", TokenType::PLUS, minimum, 0, minimum},
        {"opposite bounds cancel", TokenType::PLUS, maximum, minimum, -1},
        {"subtraction reaches maximum", TokenType::MINUS, maximum - 1, -1, maximum},
        {"subtraction reaches minimum", TokenType::MINUS, minimum + 1, 1, minimum},
        {"maximum minus zero", TokenType::MINUS, maximum, 0, maximum},
        {"minimum minus zero", TokenType::MINUS, minimum, 0, minimum},
        {"minus one minus maximum", TokenType::MINUS, -1, maximum, minimum},
        {"minimum minus minimum", TokenType::MINUS, minimum, minimum, 0},
        {"maximum minus maximum", TokenType::MINUS, maximum, maximum, 0},
        {"positive times positive", TokenType::MULTIPLICATION, 7, 3, 21},
        {"positive times negative", TokenType::MULTIPLICATION, 7, -3, -21},
        {"negative times positive", TokenType::MULTIPLICATION, -7, 3, -21},
        {"negative times negative", TokenType::MULTIPLICATION, -7, -3, 21},
        {"minimum times one", TokenType::MULTIPLICATION, minimum, 1, minimum},
        {"one times minimum", TokenType::MULTIPLICATION, 1, minimum, minimum},
        {"maximum times one", TokenType::MULTIPLICATION, maximum, 1, maximum},
        {"one times maximum", TokenType::MULTIPLICATION, 1, maximum, maximum},
        {"maximum times minus one", TokenType::MULTIPLICATION, maximum, -1, -maximum},
        {"minus one times maximum", TokenType::MULTIPLICATION, -1, maximum, -maximum},
        {"multiplication reaches minimum", TokenType::MULTIPLICATION,
            minimum / 2, 2, minimum},
        {"positive product near maximum", TokenType::MULTIPLICATION,
            maximum / 2, 2, maximum - 1},
        {"negative product near minimum", TokenType::MULTIPLICATION,
            -(maximum / 2), 2, -maximum + 1},
        {"two negatives near maximum", TokenType::MULTIPLICATION,
            -(maximum / 2), -2, maximum - 1},
        {"minimum times zero", TokenType::MULTIPLICATION, minimum, 0, 0},
        {"zero times minimum", TokenType::MULTIPLICATION, 0, minimum, 0},
        {"maximum times zero", TokenType::MULTIPLICATION, maximum, 0, 0},
        {"zero times maximum", TokenType::MULTIPLICATION, 0, maximum, 0},
        {"minimum divided by one", TokenType::DIVISION, minimum, 1, minimum},
        {"maximum divided by one", TokenType::DIVISION, maximum, 1, maximum},
        {"maximum divided by minus one", TokenType::DIVISION, maximum, -1, -maximum},
        {"minimum divided by minimum", TokenType::DIVISION, minimum, minimum, 1},
        {"positive quotient truncates", TokenType::DIVISION, 7, 3, 2},
        {"negative numerator truncates toward zero", TokenType::DIVISION, -7, 3, -2},
        {"negative denominator truncates toward zero", TokenType::DIVISION, 7, -3, -2},
        {"two negatives truncate toward zero", TokenType::DIVISION, -7, -3, 2},
        {"zero divided by minimum", TokenType::DIVISION, 0, minimum, 0},
    };
    for (const auto& test : valid_cases) {
        checks.expect_value(test.name, [&] {
            return evaluator.evalInfixExpression(Token{test.op, ""},
                Value{test.left}, Value{test.right});
        }, Value{test.expected});
    }

    // Integer bounds apply only when both operands are integers. Keep the
    // existing floating-point result types and promotion at those bounds.
    struct NumericCase {
        const char* name;
        TokenType op;
        Value left;
        Value right;
        Value expected;
    };
    const NumericCase promoted_cases[] = {
        {"float sum exceeds integer maximum", TokenType::PLUS,
            float(maximum), float(maximum), float(maximum) * 2.0f},
        {"double sum exceeds integer maximum", TokenType::PLUS,
            double(maximum), double(maximum), double(maximum) * 2.0},
        {"int minus float exceeds integer bounds", TokenType::MINUS,
            minimum, float(maximum), float(minimum) - float(maximum)},
        {"double minus int exceeds integer bounds", TokenType::MINUS,
            double(minimum), maximum, double(minimum) - double(maximum)},
        {"float times int exceeds integer bounds", TokenType::MULTIPLICATION,
            2.0f, maximum, 2.0f * float(maximum)},
        {"int times double exceeds integer bounds", TokenType::MULTIPLICATION,
            minimum, -1.0, -double(minimum)},
        {"minimum divided by float minus one", TokenType::DIVISION,
            minimum, -1.0f, -float(minimum)},
        {"double minimum divided by int minus one", TokenType::DIVISION,
            double(minimum), -1, -double(minimum)},
    };
    for (const auto& test : promoted_cases) {
        checks.expect_value(test.name, [&] {
            return evaluator.evalInfixExpression(Token{test.op, ""}, test.left, test.right);
        }, test.expected);
    }

    struct ZeroDivisionCase { Value left; Value right; };
    const ZeroDivisionCase floating_zero_cases[] = {
        {1.0f, 0.0f}, {1.0, -0.0}, {1, -0.0f}, {1.0f, 0},
        {1, 0.0}, {1.0, 0}, {1.0f, -0.0}, {1.0, 0.0f},
    };
    for (const auto& test : floating_zero_cases) {
        checks.expect_runtime_error("floating or mixed division by zero: "
            + describe(test.left) + ", " + describe(test.right), [&] {
            return evaluator.evalInfixExpression(Token{TokenType::DIVISION, "/"},
                test.left, test.right);
        }, "divide by zero");
    }

    struct OverflowCase {
        const char* name;
        TokenType op;
        int left;
        int right;
    };
    const OverflowCase overflow_cases[] = {
        {"maximum plus one overflows", TokenType::PLUS, maximum, 1},
        {"one plus maximum overflows", TokenType::PLUS, 1, maximum},
        {"minimum plus minus one overflows", TokenType::PLUS, minimum, -1},
        {"minus one plus minimum overflows", TokenType::PLUS, -1, minimum},
        {"maximum plus maximum overflows", TokenType::PLUS, maximum, maximum},
        {"minimum plus minimum overflows", TokenType::PLUS, minimum, minimum},
        {"minimum minus one overflows", TokenType::MINUS, minimum, 1},
        {"maximum minus minus one overflows", TokenType::MINUS, maximum, -1},
        {"zero minus minimum overflows", TokenType::MINUS, 0, minimum},
        {"maximum minus minimum overflows", TokenType::MINUS, maximum, minimum},
        {"minimum minus maximum overflows", TokenType::MINUS, minimum, maximum},
        {"maximum times two overflows", TokenType::MULTIPLICATION, maximum, 2},
        {"two times maximum overflows", TokenType::MULTIPLICATION, 2, maximum},
        {"minimum times two overflows", TokenType::MULTIPLICATION, minimum, 2},
        {"two times minimum overflows", TokenType::MULTIPLICATION, 2, minimum},
        {"maximum times minus two overflows", TokenType::MULTIPLICATION, maximum, -2},
        {"minus two times maximum overflows", TokenType::MULTIPLICATION, -2, maximum},
        {"minimum times minus one overflows", TokenType::MULTIPLICATION, minimum, -1},
        {"minus one times minimum overflows", TokenType::MULTIPLICATION, -1, minimum},
        {"minimum times minimum overflows", TokenType::MULTIPLICATION, minimum, minimum},
        {"maximum times maximum overflows", TokenType::MULTIPLICATION, maximum, maximum},
        {"minimum divided by minus one overflows", TokenType::DIVISION, minimum, -1},
    };
    for (const auto& test : overflow_cases) {
        checks.expect_runtime_error(test.name, [&] {
            return evaluator.evalInfixExpression(Token{test.op, ""},
                Value{test.left}, Value{test.right});
        }, "Integer overflow");
    }
    for (const int numerator : {minimum, -1, 0, 1, maximum}) {
        checks.expect_runtime_error("integer division by zero: " + std::to_string(numerator), [&] {
            return evaluator.evalInfixExpression(Token{TokenType::DIVISION, "/"},
                Value{numerator}, Value{0});
        }, "divide by zero");
    }

    for (const int operand : {minimum + 1, maximum, 0}) {
        checks.expect_value("unary minus at valid bound: " + std::to_string(operand), [&] {
            const auto expression = prefix(TokenType::MINUS, integer(operand));
            return evaluator.eval(*expression, environment);
        }, Value{-operand});
    }
    checks.expect_runtime_error("unary minus rejects minimum", [&] {
        const auto expression = prefix(TokenType::MINUS, integer(minimum));
        return evaluator.eval(*expression, environment);
    }, "Integer overflow");

    // Exercise the six source-level reproductions without requiring an
    // out-of-range positive literal to represent the minimum integer.
    const std::string maximum_source = std::to_string(maximum);
    const std::string minimum_source = "(" + std::to_string(minimum + 1) + " - 1)";
    struct SourceCase { std::string source; std::string_view message; };
    const SourceCase source_cases[] = {
        {"1 / 0;", "divide by zero"},
        {maximum_source + " + 1;", "Integer overflow"},
        {minimum_source + " - 1;", "Integer overflow"},
        {maximum_source + " * 2;", "Integer overflow"},
        {"-" + minimum_source + ";", "Integer overflow"},
        {minimum_source + " / -1;", "Integer overflow"},
    };
    for (const auto& test : source_cases) {
        checks.expect_runtime_error("source: " + test.source, [&] {
            Parser parser(lexxer{test.source});
            const auto program = parser.ParseProgram();
            return evaluator.eval(*program, environment);
        }, test.message);
    }
}

void check_if_else(Evaluator& evaluator, Environment& environment, Checks& checks) {
    // IfStatement is a Statement in this AST, so test it separately from the
    // expression-only cases. Selected blocks return their last expression.
    struct IfCase {
        const char* name;
        std::unique_ptr<IfStatement> node;
        Value expected;
    };
    const IfCase cases[] = {
        {"if: true selects consequence", conditional(boolean(true),
            block(statement(integer(10))), block(statement(integer(20)))), 10},
        {"if: false selects alternative", conditional(boolean(false),
            block(statement(integer(10))), block(statement(integer(20)))), 20},
        {"if: true works without else", conditional(boolean(true),
            block(statement(integer(7)))), 7},
        {"if: evaluates a true comparison", conditional(
            infix(TokenType::LESS, integer(2), integer(3)),
            block(statement(integer(10))), block(statement(integer(20)))), 10},
        {"if: evaluates a false comparison", conditional(
            infix(TokenType::GREATER, integer(2), integer(3)),
            block(statement(integer(10))), block(statement(integer(20)))), 20},
        {"if: evaluates a boolean prefix condition", conditional(
            prefix(TokenType::NOT, boolean(true)),
            block(statement(integer(10))), block(statement(integer(20)))), 20},
        {"if: evaluates a mixed numeric comparison", conditional(
            infix(TokenType::LESS, floating(1.25f), integer(2)),
            block(statement(integer(10))), block(statement(integer(20)))), 10},
        {"if: consequence preserves float type", conditional(boolean(true),
            block(statement(floating(1.25f))), block(statement(integer(0)))), 1.25f},
        {"if: alternative preserves float type", conditional(boolean(false),
            block(statement(integer(0))), block(statement(floating(2.5f)))), 2.5f},
        {"if: preserves a false boolean result", conditional(boolean(true),
            block(statement(boolean(false))), block(statement(integer(0)))), false},
        {"if: preserves an integer zero result", conditional(boolean(false),
            block(statement(boolean(true))), block(statement(integer(0)))), 0},
        {"if: skips an error in the alternative", conditional(boolean(true),
            block(statement(integer(42))),
            block(statement(prefix(TokenType::NOT, integer(1))))), 42},
        {"if: skips an error in the consequence", conditional(boolean(false),
            block(statement(prefix(TokenType::NOT, integer(1)))),
            block(statement(integer(24)))), 24},
        {"if: nested conditional in consequence", conditional(boolean(true),
            block(conditional(boolean(false), block(statement(integer(10))),
                block(statement(integer(20))))), block(statement(integer(30)))), 20},
        {"if: nested conditional in alternative", conditional(boolean(false),
            block(statement(integer(10))), block(conditional(boolean(true),
                block(statement(integer(20))), block(statement(integer(30)))))), 20},
        {"if: selected block returns its last result", conditional(boolean(true),
            block(statement(integer(1)), statement(floating(2.5f))),
            block(statement(integer(0)))), 2.5f},
    };
    for (const auto& test : cases) {
        checks.expect_value(test.name, [&] {
            return execute_ordinary_statement(evaluator, *test.node, environment);
        }, test.expected);
    }

    // Conditions must be boolean; numeric truthiness is not a language rule.
    constexpr std::string_view condition_error =
        "Condition of If/Else statement must be able to be evaluated to a boolean";
    for (const int value : {0, 1}) {
        checks.expect_runtime_error("if: rejects integer condition " + std::to_string(value), [&] {
            const auto node = conditional(integer(value), block(statement(integer(10))),
                block(statement(integer(20))));
            return execute_ordinary_statement(evaluator, *node, environment);
        }, condition_error);
    }
    checks.expect_runtime_error("if: rejects float condition", [&] {
        const auto node = conditional(floating(1.0f), block(statement(integer(10))),
            block(statement(integer(20))));
        return execute_ordinary_statement(evaluator, *node, environment);
    }, condition_error);

    // Match the existing operand error so a generic unsupported-IfStatement
    // error cannot accidentally satisfy these propagation checks.
    constexpr std::string_view operand_error = "Invalid right expression for NOT";
    checks.expect_runtime_error("if: propagates a condition evaluation error", [&] {
        const auto node = conditional(prefix(TokenType::NOT, integer(1)),
            block(statement(integer(10))), block(statement(integer(20))));
        return execute_ordinary_statement(evaluator, *node, environment);
    }, operand_error);
    checks.expect_runtime_error("if: propagates a selected alternative error", [&] {
        const auto node = conditional(boolean(false), block(statement(integer(10))),
            block(statement(prefix(TokenType::NOT, integer(1)))));
        return execute_ordinary_statement(evaluator, *node, environment);
    }, operand_error);
    checks.expect_runtime_error("if: selected block evaluates earlier statements", [&] {
        const auto node = conditional(boolean(true),
            block(statement(prefix(TokenType::NOT, integer(1))), statement(integer(42))),
            block(statement(integer(20))));
        return execute_ordinary_statement(evaluator, *node, environment);
    }, operand_error);

    Program program;
    program.statements.push_back(conditional(boolean(false), block(statement(integer(1))),
        block(statement(integer(2)))));
    program.statements.push_back(statement(integer(99)));
    checks.expect_value("if: program continues after a conditional", [&] {
        return evaluator.eval(program, environment);
    }, Value{99});
}

Value evaluate_returning_function(std::unique_ptr<BlockStatements> body,
                                  TokenType return_type,
                                  std::unique_ptr<Statement> after_call = nullptr) {
    // Each case gets a fresh evaluator and environment so they do not retain
    // another case's function declaration. Return is only valid in a function.
    Evaluator evaluator;
    Environment environment;
    Program program;
    const Token name{TokenType::IDENTIFIER, "return_test"};
    auto function = std::make_unique<FunctionLiteral>(name);
    function->name = std::make_unique<Identifier>(name, name.text);
    function->type = return_type;
    function->FunctionBody = std::move(body);
    program.statements.push_back(std::move(function));

    auto call = std::make_unique<CallExpression>(
        Token{TokenType::LPARENT, "("}, std::make_unique<Identifier>(name, name.text));
    call->arguments.emplace();
    program.statements.push_back(statement(std::move(call)));
    if (after_call) {
        program.statements.push_back(std::move(after_call));
    }
    return evaluator.eval(program, environment);
}

void check_returns(Checks& checks) {
    // Follow todo.md:95-96: evaluate returns inside functions, propagate them
    // through blocks/conditionals, and reject returns outside a function.
    struct ReturnCase {
        const char* name;
        std::unique_ptr<BlockStatements> body;
        TokenType return_type;
        Value expected;
    };
    ReturnCase cases[] = {
        {"return: integer literal", block(return_statement(integer(42))),
            TokenType::INT, 42},
        {"return: zero stops execution", block(return_statement(integer(0)),
            statement(integer(99))), TokenType::INT, 0},
        {"return: boolean true retains its type", block(return_statement(boolean(true))),
            TokenType::BOOL, true},
        {"return: false stops execution", block(return_statement(boolean(false)),
            statement(boolean(true))), TokenType::BOOL, false},
        {"return: evaluates arithmetic expression", block(return_statement(
            infix(TokenType::PLUS, integer(2),
                infix(TokenType::MULTIPLICATION, integer(3), integer(4))))),
            TokenType::INT, 14},
        {"return: evaluates comparison expression", block(return_statement(
            infix(TokenType::LESS, integer(2), integer(3)))), TokenType::BOOL, true},
        {"return: skips an unreachable operand error", block(return_statement(integer(7)),
            statement(prefix(TokenType::NOT, integer(1)))), TokenType::INT, 7},
        {"return: follows earlier statements and skips later ones", block(
            statement(integer(1)), return_statement(integer(7)), statement(integer(99))),
            TokenType::INT, 7},
        {"return: exits nested blocks", block(block(return_statement(integer(7)),
            statement(prefix(TokenType::NOT, integer(1)))), statement(integer(99))),
            TokenType::INT, 7},
        {"return: exits from consequence", block(conditional(boolean(true),
            block(return_statement(integer(7))), block(statement(integer(8)))),
            statement(integer(99))), TokenType::INT, 7},
        {"return: exits from alternative", block(conditional(boolean(false),
            block(statement(integer(7))), block(return_statement(integer(8)))),
            statement(integer(99))), TokenType::INT, 8},
        {"return: propagates through nested conditionals", block(conditional(boolean(true),
            block(conditional(boolean(false), block(return_statement(integer(7))),
                block(return_statement(integer(8))))), block(statement(integer(9)))),
            statement(integer(99))), TokenType::INT, 8},
        {"return: ignores a return in an unselected branch", block(
            conditional(boolean(false), block(return_statement(integer(1)))),
            return_statement(integer(2))), TokenType::INT, 2},
    };
    for (auto& test : cases) {
        checks.expect_value(test.name, [&] {
            return evaluate_returning_function(std::move(test.body), test.return_type);
        }, test.expected);
    }

    checks.expect_value("return: caller continues after function returns", [] {
        return evaluate_returning_function(block(return_statement(integer(7))),
            TokenType::INT, statement(integer(99)));
    }, Value{99});

    constexpr std::string_view operand_error = "Invalid right expression for NOT";
    checks.expect_runtime_error("return: propagates an error in its expression", [] {
        return evaluate_returning_function(
            block(return_statement(prefix(TokenType::NOT, integer(1)))), TokenType::BOOL);
    }, operand_error);
    checks.expect_runtime_error("return: does not skip an earlier error", [] {
        return evaluate_returning_function(block(statement(prefix(TokenType::NOT, integer(1))),
            return_statement(integer(7))), TokenType::INT);
    }, operand_error);

    // Require a context error rather than accepting today's generic
    // unsupported-node exception as successful return validation.
    constexpr std::string_view context_error = "Return should be in function";
    checks.expect_runtime_error("return: rejects a standalone return outside a function", [] {
        Evaluator evaluator;
        Environment environment;
        Program program;
        program.statements.push_back(return_statement(integer(7)));
        return evaluator.eval(program, environment);
    }, context_error);
    checks.expect_runtime_error("return: rejects a block return outside a function", [] {
        Evaluator evaluator;
        Environment environment;
        Program program;
        program.statements.push_back(block(return_statement(integer(7))));
        return evaluator.eval(program, environment);
    }, context_error);
    checks.expect_runtime_error("return: rejects a program return outside a function", [] {
        Evaluator evaluator;
        Environment environment;
        Program program;
        program.statements.push_back(statement(integer(1)));
        program.statements.push_back(return_statement(integer(7)));
        return evaluator.eval(program, environment);
    }, context_error);
}

} // namespace

int main() {
    Evaluator evaluator;
    Environment environment;
    Checks checks;

    // Check literal types and recursive evaluation through the actual AST.
    struct AstCase {
        const char* name;
        std::unique_ptr<Expression> expression;
        Value expected;
    };
    const AstCase ast_cases[] = {
        {"float literal", floating(1.25f), 1.25f},
        {"float zero retains its type", floating(0.0f), 0.0f},
        {"negative stored float literal", floating(-1.25f), -1.25f},
        {"integer unary minus", prefix(TokenType::MINUS, integer(5)), -5},
        {"negating a negative integer", prefix(TokenType::MINUS, integer(-5)), 5},
        {"nested unary minus", prefix(TokenType::MINUS,
            prefix(TokenType::MINUS, integer(5))), 5},
        {"negating true", prefix(TokenType::NOT, boolean(true)), false},
        {"negating false", prefix(TokenType::NOT, boolean(false)), true},
        {"nested boolean negation", prefix(TokenType::NOT,
            prefix(TokenType::NOT, boolean(true))), true},
        {"nested multiplication and addition", infix(TokenType::PLUS, integer(2),
            infix(TokenType::MULTIPLICATION, integer(3), integer(4))), 14},
        {"negating a compound expression", prefix(TokenType::MINUS,
            infix(TokenType::PLUS, integer(2), integer(3))), -5},
        {"nested mixed arithmetic", infix(TokenType::MULTIPLICATION,
            infix(TokenType::PLUS, integer(2), floating(0.5f)), integer(4)), 10.0f},
        {"negating a comparison", prefix(TokenType::NOT,
            infix(TokenType::LESS, integer(2), floating(3.5f))), false},
    };
    for (const auto& test : ast_cases) {
        checks.expect_value(test.name, [&] { return evaluator.eval(*test.expression, environment); },
                            test.expected);
    }

    struct InfixCase {
        const char* name;
        TokenType op;
        Value left;
        Value right;
        Value expected;
    };
    const InfixCase infix_cases[] = {
        {"integer addition", TokenType::PLUS, 2, 3, 5},
        {"integer subtraction", TokenType::MINUS, 2, 5, -3},
        {"integer multiplication", TokenType::MULTIPLICATION, -2, 3, -6},
        {"integer division truncates", TokenType::DIVISION, 7, 2, 3},
        {"negative numerator division", TokenType::DIVISION, -7, 2, -3},
        {"negative denominator division", TokenType::DIVISION, 7, -2, -3},
        {"two negative division operands", TokenType::DIVISION, -7, -2, 3},
        {"small negative quotient", TokenType::DIVISION, -1, 2, 0},
        {"float addition", TokenType::PLUS, 1.25f, 2.5f, 3.75f},
        {"float subtraction", TokenType::MINUS, 1.25f, 2.5f, -1.25f},
        {"float multiplication", TokenType::MULTIPLICATION, 1.5f, 2.5f, 3.75f},
        {"float division", TokenType::DIVISION, 7.0f, 2.0f, 3.5f},
        {"float rounding tolerance", TokenType::PLUS, 0.1f, 0.2f, 0.3f},
        {"double addition", TokenType::PLUS, 1.25, 2.5, 3.75},
        {"double subtraction", TokenType::MINUS, 1.25, 2.5, -1.25},
        {"double multiplication", TokenType::MULTIPLICATION, 1.5, 2.5, 3.75},
        {"double division", TokenType::DIVISION, 7.0, 2.0, 3.5},
        {"double rounding tolerance", TokenType::PLUS, 0.1, 0.2, 0.3},
        {"int plus float", TokenType::PLUS, 2, 0.5f, 2.5f},
        {"float plus int", TokenType::PLUS, 0.5f, 2, 2.5f},
        {"int minus float", TokenType::MINUS, 2, 0.5f, 1.5f},
        {"float minus int", TokenType::MINUS, 0.5f, 2, -1.5f},
        {"int times float", TokenType::MULTIPLICATION, 2, 1.5f, 3.0f},
        {"float times int", TokenType::MULTIPLICATION, 1.5f, 2, 3.0f},
        {"int divided by float", TokenType::DIVISION, 7, 2.0f, 3.5f},
        {"float divided by int", TokenType::DIVISION, 7.0f, 2, 3.5f},
        {"int plus double", TokenType::PLUS, 2, 0.5, 2.5},
        {"double plus int", TokenType::PLUS, 0.5, 2, 2.5},
        {"float plus double", TokenType::PLUS, 1.25f, 2.5, 3.75},
        {"double plus float", TokenType::PLUS, 2.5, 1.25f, 3.75},
        {"integer less than", TokenType::LESS, 2, 3, true},
        {"integer greater than", TokenType::GREATER, 3, 2, true},
        {"equal integers are not less", TokenType::LESS, 3, 3, false},
        {"equal integers are not greater", TokenType::GREATER, 3, 3, false},
        {"mixed less than", TokenType::LESS, 2, 2.5f, true},
        {"mixed greater than", TokenType::GREATER, 2.5f, 2, true},
        {"double less than float", TokenType::LESS, 1.5, 2.0f, true},
        {"double greater than float", TokenType::GREATER, 1.5, 2.0f, false},
        {"integer equality", TokenType::COMPARE, 3, 3, true},
        {"unequal integers", TokenType::COMPARE, 3, 4, false},
        {"float equality", TokenType::COMPARE, 1.25f, 1.25f, true},
        {"unequal floats", TokenType::COMPARE, 1.25f, 1.5f, false},
        {"mixed numeric equality", TokenType::COMPARE, 2, 2.0f, true},
        {"mixed floating equality", TokenType::COMPARE, 1.25f, 1.25, true},
        {"true equals true", TokenType::COMPARE, true, true, true},
        {"false equals false", TokenType::COMPARE, false, false, true},
        {"unequal booleans", TokenType::COMPARE, true, false, false},
    };
    for (const auto& test : infix_cases) {
        checks.expect_value(test.name, [&] {
            return evaluator.evalInfixExpression(Token{test.op, ""}, test.left, test.right);
        }, test.expected);
    }

    // Check numeric pairs through addition; booleans must be rejected on either side.
    const Value numeric_inputs[] = {2, 2.5f, 4.0, true};
    const Value expected_sums[3][3] = {
        {4, 4.5f, 6.0},
        {4.5f, 5.0f, 6.5},
        {6.0, 6.5, 8.0},
    };
    for (int left = 0; left < 4; ++left) {
        for (int right = 0; right < 4; ++right) {
            const std::string name = "numeric pair " + describe(numeric_inputs[left])
                + ", " + describe(numeric_inputs[right]);
            const auto add_pair = [&] {
                return evaluator.evalInfixExpression(Token{TokenType::PLUS, "+"},
                    numeric_inputs[left], numeric_inputs[right]);
            };
            if (left == 3 || right == 3) {
                checks.expect_runtime_error(name, add_pair);
            } else {
                checks.expect_value(name, add_pair, expected_sums[left][right]);
            }
        }
    }

    const TokenType numeric_operators[] = {
        TokenType::PLUS, TokenType::MINUS, TokenType::MULTIPLICATION,
        TokenType::DIVISION, TokenType::LESS, TokenType::GREATER,
    };
    struct InvalidPair { Value left; Value right; };
    const InvalidPair invalid_pairs[] = {
        {true, 2}, {2, true}, {true, false}, {false, 2.5f}, {2.5, true},
    };
    for (const auto op : numeric_operators) {
        for (const auto& pair : invalid_pairs) {
            const std::string name = "operator " + std::to_string(static_cast<int>(op))
                + " rejects " + describe(pair.left) + ", " + describe(pair.right);
            checks.expect_runtime_error(name, [&] {
                return evaluator.evalInfixExpression(Token{op, ""}, pair.left, pair.right);
            });
        }
    }

    checks.expect_runtime_error("unary minus rejects bool", [&] {
        const auto expression = prefix(TokenType::MINUS, boolean(true));
        return evaluator.eval(*expression, environment);
    });
    for (const Value& operand : {Value{1}, Value{1.0f}, Value{1.0}}) {
        checks.expect_runtime_error("logical negation rejects " + describe(operand), [&] {
            return evaluator.evalPrefixExpression(Token{TokenType::NOT, "!"}, operand);
        });
    }
    checks.expect_runtime_error("unsupported prefix operator", [&] {
        return evaluator.evalPrefixExpression(Token{TokenType::PLUS, "+"}, Value{2});
    });
    checks.expect_runtime_error("unsupported infix operator", [&] {
        return evaluator.evalInfixExpression(Token{TokenType::EQUAL, "="}, Value{2}, Value{3});
    });
    checks.expect_runtime_error("nested operand error propagates", [&] {
        const auto expression = infix(TokenType::PLUS, integer(2),
            prefix(TokenType::NOT, integer(3)));
        return evaluator.eval(*expression, environment);
    });

    Program program;
    auto statement = std::make_unique<ExpressionStatement>(Token{TokenType::FLOAT, "1.25"});
    statement->expr = floating(1.25f);
    program.statements.push_back(std::move(statement));
    checks.expect_value("program preserves a float result", [&] {
        return evaluator.eval(program, environment);
    }, Value{1.25f});
    checks.expect_value("evaluating the same AST again preserves its value", [&] {
        return evaluator.eval(program, environment);
    }, Value{1.25f});
    checks.expect_value("evaluator remains usable after errors", [&] {
        return evaluator.evalInfixExpression(Token{TokenType::PLUS, "+"}, Value{2}, Value{3});
    }, Value{5});

    check_integer_arithmetic(evaluator, environment, checks);
    check_if_else(evaluator, environment, checks);
    check_returns(checks);

    return checks.finish();
}
