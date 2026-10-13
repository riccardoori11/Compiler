# Evaluator tests

Two executables test the current `evaluator.hpp`:

- `evaluator_tests.cpp` runs 18 checks for integer and boolean literals,
  expression statements, statement lists, programs, and unsupported nodes.
- `evaluator_operator_tests.cpp` runs 116 checks for float literals, recursive
  prefix/infix evaluation, all seven infix operators, integer division,
  float/double arithmetic, mixed numeric operands, boolean equality, numeric
  operand validation, error propagation, and reuse after an error.

The tests check runtime types as well as values. Float and double results allow
a small rounding tolerance. AST nodes are constructed directly to keep these
tests independent from the lexer and parser. Double operands are passed to
`evalInfixExpression()` directly because there is no double-literal AST node.

From the project root (`/home/riccardo/compiler`), run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --target evaluator_tests evaluator_operator_tests
ctest --test-dir build -R '^evaluator(_operator)?_tests$' --output-on-failure
```

CTest should report `100% tests passed` for two executables containing 134
checks. Build again after changing the evaluator or tests.

To see every check's result, run the executable directly after building:

```sh
./build/evaluator_tests
./build/evaluator_operator_tests
```

Their summaries should report `18/18 evaluator tests passed` and
`116/116 evaluator operator checks passed`. A failed check prints `FAIL` and
makes the executable return a nonzero exit status. The checks run in both Debug
and Release builds.

The active suite covers implemented evaluator features. It does not test the
REPL or parser, floating unary minus, or boolean/numeric equality rejection.
Division by zero and integer overflow need evaluator guards before adding
in-process rejection tests, since the current operations can crash or invoke
undefined behavior. Declaration, block, and function execution are also outside
the active suite.

## Planned language rule tests

`language_rules_tests.hpp` contains 65 cases for section 1 of `todo.md`, including
arithmetic, runtime type checks, overflow, bindings, and function calls. This
broader suite still needs environments, statement/function execution, and
runtime error checks before it can run against the interpreter.

Later, connect it with a function passed to `language_rules_tests::run()`. That
function accepts a source string, executes it in a fresh environment, and
returns the last expression's value or an expected runtime error category.
Unexpected parsing failures must remain test failures. All source inputs use
ordinary spaces.
