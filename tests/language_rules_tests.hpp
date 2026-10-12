#pragma once

#include <exception>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Behavioral tests for todo.md section 1. The interpreter adapter runs each
// source in a fresh environment and returns its last expression's value.
namespace language_rules_tests {

enum class Error {
    unsupported_type,
    type_mismatch,
    division_by_zero,
    integer_overflow,
    uninitialized_read,
    duplicate_declaration,
};

using Outcome = std::variant<int, bool, Error>;

struct TestCase {
    const char* name;
    std::string source;
    Outcome expected;
};

inline std::vector<TestCase> cases() {
    const std::string maximum = std::to_string(std::numeric_limits<int>::max());
    // Avoid a positive magnitude larger than int can hold when parsing INT_MIN.
    const std::string minimum = "(-" + maximum + " - 1)";

    return {
        {"integer value", "42;", 42},
        {"zero stays an integer", "0;", 0},
        {"true value", "true;", true},
        {"false value", "false;", false},
        {"unsupported double declaration", "double x = 1; x;", Error::unsupported_type},
        {"unsupported uninitialized double", "double x; 0;", Error::unsupported_type},

        {"integer addition", "2 + 3;", 5},
        {"integer subtraction", "2 - 5;", -3},
        {"integer multiplication", "-2 * 3;", -6},
        {"arithmetic precedence", "2 + 3 * 4;", 14},
        {"grouping and unary minus", "-(2 + 3);", -5},
        {"integer less than", "2 < 3;", true},
        {"integer greater than", "3 > 2;", true},
        {"equal values are not ordered", "3 < 3;", false},
        {"integer equality", "3 == 3;", true},
        {"unequal integers", "3 == 4;", false},
        {"boolean equality", "true == true;", true},
        {"unequal booleans", "true == false;", false},
        {"boolean negation", "!true;", false},
        {"negating false", "!false;", true},
        {"integer boolean equality mismatch", "1 == true;", Error::type_mismatch},
        {"boolean integer equality mismatch", "false == 0;", Error::type_mismatch},
        {"mixed arithmetic types", "1 + true;", Error::type_mismatch},
        {"boolean arithmetic", "true + false;", Error::type_mismatch},
        {"boolean ordering", "false < true;", Error::type_mismatch},
        {"integer logical negation", "!1;", Error::type_mismatch},
        {"boolean unary minus", "-true;", Error::type_mismatch},
        {"boolean if condition", "if (true) { int x = 1; } 2;", 2},
        {"false if condition", "if (false) { int x = 1; } 2;", 2},
        {"integer if condition", "if (1) {} 2;", Error::type_mismatch},
        {"zero if condition", "if (0) {} 2;", Error::type_mismatch},

        {"positive integer division", "7 / 2;", 3},
        {"negative numerator truncates toward zero", "-7 / 2;", -3},
        {"negative denominator truncates toward zero", "7 / -2;", -3},
        {"two negative division operands", "-7 / -2;", 3},
        {"small negative quotient truncates to zero", "-1 / 2;", 0},
        {"division by zero", "1 / 0;", Error::division_by_zero},
        {"zero divided by zero", "0 / 0;", Error::division_by_zero},
        {"maximum integer remains valid", maximum + " + 0;", std::numeric_limits<int>::max()},
        {"minimum integer remains valid", minimum + " / 1;", std::numeric_limits<int>::min()},
        {"addition overflow", maximum + " + 1;", Error::integer_overflow},
        {"subtraction underflow", minimum + " - 1;", Error::integer_overflow},
        {"multiplication overflow", maximum + " * 2;", Error::integer_overflow},
        {"unary minus overflow", "-" + minimum + ";", Error::integer_overflow},
        {"division overflow", minimum + " / -1;", Error::integer_overflow},

        {"integer initializer matches", "int x = 2 + 3; x;", 5},
        {"boolean initializer matches", "bool x = 2 < 3; x;", true},
        {"boolean initializer for integer", "int x = true; x;", Error::type_mismatch},
        {"integer initializer for boolean", "bool x = 1; x;", Error::type_mismatch},
        {"matching argument and return types", "int f(int x) { return x; } f(3);", 3},
        {"matching boolean argument and return", "bool f(bool x) { return x; } f(false);", false},
        {"boolean argument for integer parameter", "int f(int x) { return x; } f(true);", Error::type_mismatch},
        {"integer argument for boolean parameter", "bool f(bool x) { return x; } f(1);", Error::type_mismatch},
        {"boolean return from integer function", "int f() { return true; } f();", Error::type_mismatch},
        {"integer return from boolean function", "bool f() { return 1; } f();", Error::type_mismatch},

        {"unused uninitialized binding", "int x; 0;", 0},
        {"read before initialization", "int x; x;", Error::uninitialized_read},
        {"uninitialized boolean read", "bool x; x;", Error::uninitialized_read},
        {"duplicate initialized declarations", "int x = 1; int x = 2; x;", Error::duplicate_declaration},
        {"duplicate uninitialized binding", "int x; int x = 1; x;", Error::duplicate_declaration},
        {"duplicate name with different type", "int x = 1; bool x = true; x;", Error::duplicate_declaration},
        {"inner scope preserves outer value", "int x = 1; if (true) { int x = 2; } x;", 1},
        {"inner binding can have a different type", "bool x = true; if (true) { int x = 2; int y = x + 1; } x;", true},
        {"uninitialized inner binding hides outer value", "int x = 1; if (true) { int x; x; } 0;", Error::uninitialized_read},
        {"separate scopes may reuse a name", "if (true) { int x = 1; } if (true) { int x = 2; } 0;", 0},
    };
}

inline std::string describe(const Outcome& outcome) {
    if (const auto* integer = std::get_if<int>(&outcome)) {
        return "int(" + std::to_string(*integer) + ")";
    }
    if (const auto* boolean = std::get_if<bool>(&outcome)) {
        return *boolean ? "bool(true)" : "bool(false)";
    }

    switch (std::get<Error>(outcome)) {
        case Error::unsupported_type: return "unsupported type error";
        case Error::type_mismatch: return "type mismatch error";
        case Error::division_by_zero: return "division by zero error";
        case Error::integer_overflow: return "integer overflow error";
        case Error::uninitialized_read: return "uninitialized read error";
        case Error::duplicate_declaration: return "duplicate declaration error";
    }
    return "unknown error";
}

// interpret(string_view) -> Outcome must create a fresh environment per call.
// Map runtime errors to Error in the adapter; unexpected exceptions fail a test.
// Outcome comparison checks both the value and its type (int(1) != bool(true)).
template <typename Interpret>
int run(Interpret&& interpret, std::ostream& report) {
    const auto tests = cases();
    std::size_t failures = 0;
    for (const auto& test : tests) {
        try {
            const Outcome actual = interpret(std::string_view{test.source});
            if (actual == test.expected) {
                continue;
            }
            report << "FAIL: todo.md #1 (" << test.name << "): expected "
                   << describe(test.expected) << ", got " << describe(actual)
                   << "; source: " << test.source << '\n';
        } catch (const std::exception& error) {
            report << "FAIL: todo.md #1 (" << test.name << "): unexpected exception: "
                   << error.what() << "; source: " << test.source << '\n';
        }
        ++failures;
    }
    report << tests.size() - failures << '/' << tests.size() << " language rule cases passed\n";
    return failures == 0 ? 0 : 1;
}

} // namespace language_rules_tests
