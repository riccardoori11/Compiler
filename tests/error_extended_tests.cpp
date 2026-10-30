#include "evaluator.hpp"
#include "parser.hpp"
#include <functional>
#include <iostream>
#include <limits>
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

Value evaluate(std::string source) {
    const auto program = parse(std::move(source));
    Evaluator evaluator;
    Environment environment;
    return evaluator.eval(*program, environment);
}

void expect_integer(std::string source, int expected) {
    const auto result = evaluate(std::move(source));
    const auto* value = std::get_if<int>(&result);
    require(value && *value == expected, "unexpected value or runtime type");
}

template<class Action>
void expect_error(Action action, std::string_view message = {}) {
    try {
        action();
    } catch (const std::runtime_error& error) {
        require(std::string_view{error.what()}.find(message) != std::string_view::npos,
                "unexpected interpreter diagnostic");
        return;
    }
    throw std::runtime_error("expected an interpreter error");
}
}

int main() {
    struct Test { const char* name; std::function<void()> run; };
    const Test tests[] = {
        {"failed grouped callee is rejected during parsing", [] {
            expect_error([] { parse("(1 + )();"); });
        }},
        {"missing grouped delimiter is rejected during parsing", [] {
            expect_error([] { parse("(1 + 2;"); });
        }},
        {"incomplete prefix is rejected during parsing", [] {
            expect_error([] { parse("-(1 + );"); });
        }},
        {"alternative block bindings do not leak", [] {
            expect_error([] { evaluate("if (false) {} else { int x = 7; } x;"); },
                         "could not find name");
        }},
        {"conditional locals do not leak within a function", [] {
            expect_error([] { evaluate("int f() { if (true) { int x = 7; } return x; } f();"); },
                         "could not find name");
        }},
        {"nested block can shadow a parameter", [] {
            expect_integer("int f(int x) { if (true) { int x = 9; } return x; } f(7);", 7);
        }},
        {"function body shares its parameter scope", [] {
            expect_error([] { evaluate("int f(int x) { int x = 9; return x; } f(7);"); },
                         "already exsits");
        }},
        {"nested return leaves the function", [] {
            expect_integer("int f() { if (true) { if (true) { return 7; } } return 9; } f();", 7);
        }},
        {"nested shadowing preserves every enclosing binding", [] {
            expect_integer("int x = 1; if (true) { int x = 2; if (true) { int x = 3; } } x;", 1);
        }},
        {"minimum integer literal is exact", [] {
            expect_integer("-2147483648;", std::numeric_limits<int>::min());
        }},
        {"minimum literal obeys arithmetic precedence", [] {
            expect_integer("-2147483648 * 0 + 7;", 7);
        }},
        {"positive magnitude beyond integer maximum is rejected", [] {
            expect_error([] { parse("2147483648;"); }, "out of range");
        }},
        {"literal below minimum integer is rejected", [] {
            expect_error([] { parse("-2147483649;"); }, "out of range");
        }},
        {"negating minimum literal reports overflow", [] {
            expect_error([] { evaluate("-(-2147483648);"); }, "overflow");
        }},
        {"minimum literal divided by negative one reports overflow", [] {
            expect_error([] { evaluate("-2147483648 / -1;"); }, "overflow");
        }},
        {"float underflow is reported explicitly", [] {
            expect_error([] { parse("0." + std::string(60, '0') + "1;"); }, "out of range");
        }},
        {"all standard whitespace characters separate tokens", [] {
            expect_integer("\t\v\f\r\n int\tx\v=\f7;\r\nx; \t\v\f\r\n", 7);
            require(parse(" \t\v\f\r\n")->statements.empty(), "whitespace produced a statement");
        }},
        {"booleans and floats report non-callable values", [] {
            expect_error([] { evaluate("true();"); }, "not callable");
            expect_error([] { evaluate("1.5();"); }, "not callable");
        }},
        {"public type helper covers all runtime alternatives", [] {
            require(type(Value{1}) == "int", "wrong int type");
            require(type(Value{1.0f}) == "float", "wrong float type");
            require(type(Value{1.0}) == "double", "wrong double type");
            require(type(Value{true}) == "bool", "wrong bool type");
            require(type(Value{}) == "NULL", "wrong null type");
            require(type(Value{std::make_shared<FunctionObj>()}) == "function", "wrong function type");
        }},
    };
    int failures = 0;
    for (const auto& test : tests) {
        try {
            test.run();
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL: " << test.name << ": " << error.what() << '\n';
        }
    }
    std::cout << std::size(tests) - failures << '/' << std::size(tests)
              << " extended error checks passed\n";
    return failures ? 1 : 0;
}
