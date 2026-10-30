#include "evaluator.hpp"
#include "parser.hpp"
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

auto parse(std::string source) {
    Parser parser(lexxer{std::move(source)});
    return parser.ParseProgram();
}

Value evaluate(Evaluator& evaluator, Environment& environment, std::string source) {
    const auto program = parse(std::move(source));
    return evaluator.eval(*program, environment);
}

void expect_integer(const Value& value, int expected) {
    const auto* actual = std::get_if<int>(&value);
    require(actual && *actual == expected, "wrong integer result or runtime type");
}

void expect_value(std::string source, int expected) {
    Evaluator evaluator;
    Environment environment;
    expect_integer(evaluate(evaluator, environment, std::move(source)), expected);
}

template<class Action>
void expect_runtime_error(Action action, std::string_view diagnostic = {}) {
    try {
        action();
    } catch (const std::runtime_error& error) {
        require(std::string_view{error.what()}.find(diagnostic) != std::string_view::npos,
                "unexpected runtime diagnostic");
        return;
    }
    throw std::runtime_error("expected a runtime error, but operation succeeded");
}

void expect_parse_error(std::string source) {
    // Reject in the parser, before any partial program can execute.
    expect_runtime_error([&] { parse(std::move(source)); });
}

void expect_evaluation_error(std::string source, std::string_view diagnostic = {}) {
    const auto program = parse(std::move(source));
    Evaluator evaluator;
    Environment environment;
    expect_runtime_error([&] { evaluator.eval(*program, environment); }, diagnostic);
}

// Every entry runs in its own CTest process. Crashes are failures, never passes.
const std::map<std::string, std::function<void()>> cases{
    {"error_01_grouped_operand", [] { expect_parse_error("(1 + ) + 2;"); }},
    {"error_02_divide_zero", [] { expect_evaluation_error("1 / 0;"); }},
    {"error_02_add_overflow", [] { expect_evaluation_error("2147483647 + 1;"); }},
    {"error_02_subtract_overflow", [] { expect_evaluation_error("(-2147483647 - 1) - 1;"); }},
    {"error_02_multiply_overflow", [] { expect_evaluation_error("2147483647 * 2;"); }},
    {"error_02_negate_minimum", [] { expect_evaluation_error("-(-2147483647 - 1);"); }},
    {"error_02_divide_overflow", [] { expect_evaluation_error("(-2147483647 - 1) / -1;"); }},
    {"error_03_destroyed_ast", [] {
        Evaluator evaluator;
        Environment environment;
        {
            const auto program = parse("int f() { return 7; }");
            evaluator.eval(*program, environment);
        }
        expect_integer(evaluate(evaluator, environment, "f();"), 7);
    }},
    {"error_03_destroyed_environment", [] {
        Evaluator evaluator;
        const auto program = parse("int x = 7; int f() { return x; }");
        Value saved;
        {
            Environment defining;
            evaluator.eval(*program, defining);
            saved = *defining.Get("f")->value;
        }
        Environment caller;
        caller.define("saved", Binding{TokenType::FUNCTION, saved});
        expect_integer(evaluate(evaluator, caller, "saved();"), 7);
    }},
    {"error_04_print_uninitialized", [] { parse("int x;")->print(); }},
    {"error_04_print_empty", [] { parse("")->print(); }},
    {"error_05_semicolon_separator", [] {
        expect_parse_error("int f(int a, int b) { return a + b; } f(1; 2);");
    }},
    {"error_05_illegal_separator", [] {
        expect_parse_error("int f(int a, int b) { return a + b; } f(1 @ 2);");
    }},
    {"error_06_discarded_declaration", [] { expect_parse_error("int x 1;"); }},
    {"error_06_discarded_if", [] { expect_parse_error("if true;"); }},
    {"error_07_block_leak", [] {
        expect_evaluation_error("if (true) { int x = 7; } x;", "could not find name");
    }},
    {"error_07_shadowing", [] {
        expect_value("int x = 1; if (true) { int x = 2; } x;", 1);
    }},
    {"error_10_integer_range", [] {
        expect_parse_error("9999999999999999999999999999;");
    }},
    {"error_10_float_range", [] {
        expect_parse_error("9999999999999999999999999999999999999999999999999999.0;");
    }},
    {"error_10_minimum_literal", [] {
        Evaluator evaluator;
        Environment environment;
        Value result;
        try {
            result = evaluate(evaluator, environment, "-2147483648;");
        } catch (const std::runtime_error&) {
            // Either explicit rejection or an exact value is allowed until the
            // language defines minimum-literal syntax. Silent zero is not.
            return;
        }
        expect_integer(result, std::numeric_limits<int>::min());
    }},
    {"error_11_tab", [] { expect_value("int\tx = 1; x;", 1); }},
    {"error_11_newline", [] { expect_value("int x = 1;\nx;", 1); }},
    {"error_11_carriage_return", [] { expect_value("int x = 1;\rx;", 1); }},
    {"error_11_crlf", [] { expect_value("int x = 1;\r\nx;", 1); }},
    {"error_12_non_callable", [] {
        // std::bad_variant_access is not a runtime_error and must fail.
        expect_evaluation_error("int x = 1; x();");
    }},
    {"error_control_valid_call", [] {
        expect_value("int f(int a, int b) { return a + b; } f(1, 2);", 3);
    }},
    {"error_control_integer_bounds", [] {
        expect_value("2147483647;", std::numeric_limits<int>::max());
        expect_value("-2147483647 - 1;", std::numeric_limits<int>::min());
    }},
};
} // namespace

int main(int argc, char** argv) {
    static_assert(std::numeric_limits<int>::digits == 31,
                  "error.md's numeric reproductions assume 32-bit int");
    if (argc != 2 || !cases.contains(argv[1])) {
        std::cerr << "Expected one registered error regression case name\n";
        return 2;
    }
    try {
        cases.at(argv[1])();
        std::cout << "PASS: " << argv[1] << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << argv[1] << ": " << error.what() << '\n';
        return 1;
    }
}
