#include "evaluator.hpp"
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
        if constexpr (std::is_same_v<T, bool>) {
            return item ? "bool(true)" : "bool(false)";
        } else if constexpr (std::is_same_v<T, int>) {
            return "int(" + std::to_string(item) + ")";
        } else if constexpr (std::is_same_v<T, float>) {
            return "float(" + std::to_string(item) + ")";
        } else {
            return "double(" + std::to_string(item) + ")";
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
    void expect_runtime_error(std::string_view name, Evaluate evaluate) {
        ++checks;
        try {
            evaluate();
            std::cerr << "FAIL: " << name << ": expected std::runtime_error\n";
        } catch (const std::runtime_error&) {
            return;
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

} // namespace

int main() {
    Evaluator evaluator;
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
        checks.expect_value(test.name, [&] { return evaluator.eval(*test.expression); },
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

    // A boolean on either side must not count as a numeric pair.
    const Value numeric_inputs[] = {2, 2.5f, 4.0, true};
    for (int left = 0; left < 4; ++left) {
        for (int right = 0; right < 4; ++right) {
            const std::string name = "numeric pair " + describe(numeric_inputs[left])
                + ", " + describe(numeric_inputs[right]);
            checks.expect_value(name, [&] {
                return Value{evaluator.IsNumericValue(numeric_inputs[left], numeric_inputs[right])};
            }, Value{left != 3 && right != 3});
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
        return evaluator.eval(*expression);
    });
    for (const Value operand : {Value{1}, Value{1.0f}, Value{1.0}}) {
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
        return evaluator.eval(*expression);
    });

    Program program;
    auto statement = std::make_unique<ExpressionStatement>(Token{TokenType::FLOAT, "1.25"});
    statement->expr = floating(1.25f);
    program.statements.push_back(std::move(statement));
    checks.expect_value("program preserves a float result", [&] {
        return evaluator.eval(program);
    }, Value{1.25f});
    checks.expect_value("evaluating the same AST again preserves its value", [&] {
        return evaluator.eval(program);
    }, Value{1.25f});
    checks.expect_value("evaluator remains usable after errors", [&] {
        return evaluator.evalInfixExpression(Token{TokenType::PLUS, "+"}, Value{2}, Value{3});
    }, Value{5});

    return checks.finish();
}
