#include "evaluator.hpp"
#include "parser.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) {
        throw std::runtime_error(std::string{message});
    }
}

class Checks {
    int checks = 0;
    int failures = 0;

public:
    template <typename Test>
    void run(std::string_view name, Test test) {
        ++checks;
        try {
            test();
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "FAIL: " << name << ": " << error.what() << '\n';
        }
    }

    int finish() const {
        std::cout << checks - failures << '/' << checks
                  << " parser statement checks passed\n";
        return failures == 0 ? 0 : 1;
    }
};

std::unique_ptr<Program> parse(std::string source) {
    Parser parser(lexxer{std::move(source)});
    return parser.ParseProgram();
}

void expect_rejected(const char* source) {
    // Evaluation is deliberately excluded: the parser must reject the source
    // before it can return a partial program that a caller might execute.
    bool rejected = false;
    try {
        parse(source);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    require(rejected, "ParseProgram accepted an invalid statement");
}

Value evaluate(Evaluator& evaluator, Environment& environment, std::string source) {
    const auto program = parse(std::move(source));
    return evaluator.eval(*program, environment);
}

void expect_integer(const Value& value, int expected) {
    const auto* actual = std::get_if<int>(&value);
    require(actual && *actual == expected, "unexpected result or runtime type");
}

} // namespace

int main() {
    Checks checks;

    struct InvalidCase {
        const char* name;
        const char* source;
    };
    const InvalidCase invalid[] = {
        {"error 6: declaration without '=' is rejected", "int x 1;"},
        {"error 6: if without parentheses is rejected", "if true;"},
        {"int declaration requires an identifier", "int 1;"},
        {"double declaration requires an identifier", "double 1;"},
        {"bool declaration requires an identifier", "bool true;"},
        {"unfinished type is rejected", "int"},
        {"unfinished declaration is rejected", "int x"},
        {"if requires a block", "if (true); 42;"},
        {"else requires a block", "if (true) { 1; } else 42;"},
        {"bad declaration after a valid statement rejects the program",
            "int before = 1; int x 2; 42;"},
        {"bad if after a valid statement rejects the program", "1; if true; 42;"},
        {"bad declaration in a consequence is rejected",
            "if (true) { int x 1; 42; }"},
        {"bad if in a consequence is rejected", "if (true) { if true; 42; }"},
        {"bad declaration in an unselected consequence is rejected",
            "if (false) { int x 1; } 42;"},
        {"bad declaration in an unselected alternative is rejected",
            "if (true) { 42; } else { int x 1; }"},
        {"bad if in an alternative is rejected",
            "if (false) { 1; } else { if true; 42; }"},
        {"bad declaration in a function body is rejected",
            "int f() { int x 1; return 7; } 42;"},
        {"bad if in an uncalled function is rejected",
            "int f() { if true; return 7; } 42;"},
        {"bad declaration in a nested dead branch is rejected",
            "if (true) { if (false) { int x 1; } else { 2; } } 42;"},
        {"bad if in a nested function rejects the outer program",
            "int outer() { int inner() { if true; } return 1; } 42;"},
        {"return requires a value", "return;"},
        {"return in a function requires a value", "int f() { return; 7; }"},
        {"typed declaration in a function requires an identifier",
            "int f() { int 1; return 7; } 42;"},
    };
    for (const auto& test : invalid) {
        checks.run(test.name, [&] { expect_rejected(test.source); });
    }

    checks.run("empty input and spaces produce empty programs", [] {
        require(parse("")->statements.empty(), "empty source was not empty");
        require(parse("   ")->statements.empty(), "spaces were not empty");
    });

    checks.run("empty conditional blocks remain valid AST nodes", [] {
        const auto program = parse("if (true) {} else {}");
        require(program->statements.size() == 1, "conditional was discarded");
        const auto* statement = dynamic_cast<const IfStatement*>(program->statements[0].get());
        require(statement && statement->Consequence && statement->Alternative,
            "empty conditional block was treated as a failure");
        require(statement->Consequence->Block_Statements.empty()
            && statement->Alternative->Block_Statements.empty(), "block was not empty");
    });

    checks.run("empty function body remains a valid AST node", [] {
        const auto program = parse("int empty() {}");
        require(program->statements.size() == 1, "function was discarded");
        const auto* statement = dynamic_cast<const FunctionLiteral*>(program->statements[0].get());
        require(statement && statement->FunctionBody, "empty function body was rejected");
        require(statement->FunctionBody->Block_Statements.empty(), "body was not empty");
    });

    checks.run("uninitialized declarations retain null initializers", [] {
        const auto program = parse("int x; double y; bool z;");
        require(program->statements.size() == 3, "uninitialized declaration was discarded");
        for (const auto& node : program->statements) {
            const auto* declaration = dynamic_cast<const VariableDeclaration*>(node.get());
            require(declaration && declaration->name && !declaration->value,
                "uninitialized declaration was not preserved");
        }
    });

    checks.run("valid declarations and subsequent statements still execute", [] {
        Evaluator evaluator;
        Environment environment;
        expect_integer(evaluate(evaluator, environment, "int x = 1; int y = 2; x + y;"), 3);
    });

    checks.run("valid conditional statements still execute", [] {
        Evaluator evaluator;
        Environment environment;
        expect_integer(evaluate(evaluator, environment,
            "bool selected = false; if (selected) { 1; } else { 7; }"), 7);
    });

    checks.run("valid functions and arguments still execute", [] {
        Evaluator evaluator;
        Environment environment;
        expect_integer(evaluate(evaluator, environment,
            "int add(int a, int b) { return a + b; } add(2, 3);"), 5);
    });

    checks.run("valid nested blocks still execute", [] {
        Evaluator evaluator;
        Environment environment;
        expect_integer(evaluate(evaluator, environment,
            "if (true) { if (false) { 1; } else { 7; } }"), 7);
    });

    checks.run("failed parsing prevents partial execution and leaves bindings intact", [] {
        Evaluator evaluator;
        Environment environment;
        environment.define("existing", Binding{TokenType::INT, Value{7}});
        bool parsed = false;
        try {
            const auto program = parse("int before = 1; int x 2; int after = 3;");
            parsed = true;
            evaluator.eval(*program, environment);
        } catch (const std::runtime_error&) {
            require(!parsed, "failure occurred after evaluation began");
        }
        require(!parsed, "invalid source returned a partial program");
        require(!environment.Get("before") && !environment.Get("x") && !environment.Get("after"),
            "a statement from the invalid source executed");
        expect_integer(evaluate(evaluator, environment, "existing;"), 7);
    });

    return checks.finish();
}
