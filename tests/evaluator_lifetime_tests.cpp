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
                  << " evaluator lifetime checks passed\n";
        return failures == 0 ? 0 : 1;
    }
};

std::unique_ptr<Program> parse(std::string source) {
    Parser parser(lexxer{std::move(source)});
    return parser.ParseProgram();
}

Value evaluate(Evaluator& evaluator, Environment& environment, std::string source) {
    const auto program = parse(std::move(source));
    return evaluator.eval(*program, environment);
}

std::shared_ptr<FunctionObj> function(const Value& value) {
    const auto* result = std::get_if<std::shared_ptr<FunctionObj>>(&value);
    require(result && *result, "expected a function value");
    return *result;
}

Value get_value(const Environment& environment, const std::string& name) {
    const auto binding = environment.Get(name);
    require(binding && binding->value, "missing binding value");
    return *binding->value;
}

void install(Environment& environment, const Value& value) {
    environment.define("saved", Binding{TokenType::FUNCTION, value});
}

Value call(Evaluator& evaluator, const Value& value, std::string arguments = {}) {
    Environment caller;
    install(caller, value);
    return evaluate(evaluator, caller, "saved(" + arguments + ");");
}

void expect_integer(const Value& value, int expected) {
    const auto* actual = std::get_if<int>(&value);
    require(actual && *actual == expected, "unexpected result or runtime type");
}

} // namespace

int main() {
    Checks checks;

    checks.run("retained function survives its AST and defining wrapper", [] {
        Evaluator evaluator;
        Value saved;
        {
            Environment defining;
            saved = evaluate(evaluator, defining,
                "int x = 7; int f() { return x; } f;");
        }
        expect_integer(call(evaluator, saved), 7);
    });

    checks.run("capture retains its defining scope and ancestors", [] {
        Evaluator evaluator;
        Value saved;
        {
            Environment parent;
            evaluate(evaluator, parent, "int x = 30;");
            Environment child(&parent);
            saved = evaluate(evaluator, child,
                "int y = 12; int f() { return x + y; } f;");
        }
        expect_integer(call(evaluator, saved), 42);
    });

    checks.run("capture sees later definitions and binding updates", [] {
        Evaluator evaluator;
        Environment defining;
        const Value saved = evaluate(evaluator, defining,
            "int x = 1; int f() { return x + y; } f;");
        defining.Set("x", Binding{TokenType::INT, Value{20}});
        evaluate(evaluator, defining, "int y = 22;");
        expect_integer(call(evaluator, saved), 42);
    });

    checks.run("captured names resolve in the defining scope", [] {
        Evaluator evaluator;
        Environment defining;
        const Value saved = evaluate(evaluator, defining,
            "int x = 7; int f() { return x; } f;");
        Environment caller;
        evaluate(evaluator, caller, "int x = 99;");
        install(caller, saved);
        expect_integer(evaluate(evaluator, caller, "saved();"), 7);
    });

    checks.run("one AST produces independent captures and fresh call parameters", [] {
        Evaluator evaluator;
        Value first;
        Value second;
        {
            const auto program = parse("int f(int n) { return x + n; }");
            Environment first_scope;
            Environment second_scope;
            evaluate(evaluator, first_scope, "int x = 10;");
            evaluate(evaluator, second_scope, "int x = 20;");
            evaluator.eval(*program, first_scope);
            evaluator.eval(*program, second_scope);
            first = get_value(first_scope, "f");
            second = get_value(second_scope, "f");
        }
        expect_integer(call(evaluator, first, "2"), 12);
        expect_integer(call(evaluator, second, "3"), 23);
        expect_integer(call(evaluator, first, "4"), 14);
    });

    checks.run("recursive calls survive destruction of their defining wrapper", [] {
        Evaluator evaluator;
        Value saved;
        {
            Environment defining;
            saved = evaluate(evaluator, defining,
                "int f(int n) { if (n < 2) { return 1; } return n * f(n - 1); } f;");
        }
        expect_integer(call(evaluator, saved, "5"), 120);
        expect_integer(call(evaluator, saved, "3"), 6);
    });

    checks.run("nested function survives its local scope through the execution API", [] {
        Evaluator evaluator;
        Value saved;
        {
            Environment parent;
            evaluate(evaluator, parent, "int base = 2;");
            Environment local(&parent);
            evaluate(evaluator, local, "int x = 30;");
            const auto program = parse(
                "int outer() { int inner(int y) { return base + x + y; } return inner; }");
            const auto* outer = dynamic_cast<const FunctionLiteral*>(program->statements.at(0).get());
            require(outer, "missing outer declaration");
            // Source return types exclude functions. The body execution API can
            // still expose a nested function value for this lifetime check.
            const auto result = evaluator.execute(*outer->FunctionBody, local);
            require(result.is_return, "nested body did not return");
            saved = result.value;
        }
        expect_integer(call(evaluator, saved, "5"), 37);
    });

    checks.run("nested declarations use a fresh environment for each call", [] {
        Evaluator evaluator;
        Environment defining;
        const Value saved = evaluate(evaluator, defining,
            "int f(int x) { int g(int y) { return x + y; } return g(3); } f;");
        expect_integer(call(evaluator, saved, "2"), 5);
        expect_integer(call(evaluator, saved, "9"), 12);
    });

    checks.run("child environment retains a destroyed parent wrapper's bindings", [] {
        std::unique_ptr<Environment> child;
        {
            Environment parent;
            parent.define("x", Binding{TokenType::INT, Value{9}});
            child = std::make_unique<Environment>(&parent);
        }
        expect_integer(get_value(*child, "x"), 9);
    });

    checks.run("copied environment handles refer to the same scope", [] {
        Environment original;
        original.define("x", Binding{TokenType::INT, Value{1}});
        Environment alias = original;
        alias.Set("x", Binding{TokenType::INT, Value{7}});
        expect_integer(get_value(original, "x"), 7);
        original.define("y", Binding{TokenType::INT, Value{9}});
        expect_integer(get_value(alias, "y"), 9);
    });

    checks.run("runtime teardown releases captured scopes and recursive bodies", [] {
        std::weak_ptr<EnvironmentFrame> scope;
        std::weak_ptr<BlockStatements> body;
        {
            Evaluator evaluator;
            {
                Environment defining;
                const auto saved = evaluate(evaluator, defining,
                    "int f(int n) { int g() { return n; } if (n < 2) { return 1; } return g() * f(n - 1); } f;");
                const auto object = function(saved);
                scope = object->env;
                body = object->block;
                expect_integer(call(evaluator, saved, "5"), 120);
            }
            require(!scope.expired(), "runtime did not retain a captured scope");
            require(!body.expired(), "runtime lost its stored function body");
        }
        require(scope.expired(), "captured scope survived all owners");
        require(body.expired(), "function body survived all owners");
    });

    checks.run("retained function reports an expired environment after runtime teardown", [] {
        Value saved;
        {
            Evaluator evaluator;
            Environment defining;
            saved = evaluate(evaluator, defining,
                "int x = 7; int f() { return x; } f;");
        }
        require(function(saved)->env.expired(), "environment unexpectedly stayed alive");
        Evaluator evaluator;
        try {
            call(evaluator, saved);
        } catch (const std::runtime_error& error) {
            require(std::string_view{error.what()} == "Function environment has expired",
                "unexpected diagnostic for an expired capture");
            return;
        }
        throw std::runtime_error("expired capture was accepted");
    });

    checks.run("a surviving environment keeps functions callable by another evaluator", [] {
        Environment defining;
        Value saved;
        {
            Evaluator evaluator;
            saved = evaluate(evaluator, defining,
                "int x = 7; int f() { return x; } f;");
        }
        Evaluator evaluator;
        expect_integer(call(evaluator, saved), 7);
    });

    return checks.finish();
}
