# Evaluator tests

Two executables test the current `evaluator.hpp`:

- `evaluator_tests.cpp` runs 18 checks for integer and boolean literals,
  expression statements, statement lists, programs, and unsupported nodes.
- `evaluator_operator_tests.cpp` runs 139 checks for float literals, recursive
  prefix/infix evaluation, all seven infix operators, integer division,
  float/double arithmetic, mixed numeric operands, boolean equality, numeric
  operand validation, error propagation, and reuse after an error. This includes
  23 new conditional checks for branch selection, computed boolean conditions,
  skipped branches, nested conditionals, branch result types, multi-statement
  blocks, invalid condition types, and continued program execution.

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

The two executables contain 157 checks. Build again after changing the evaluator
or tests.

The project builds with `std::monostate`, including the test diagnostic helper's
null case. Currently 28 operator checks fail: three greater-than cases, three
boolean equality cases, and 22 conditional cases. `isTruthy()` rejects booleans
instead of non-booleans, and `eval()` still has no `BlockStatements` handling.
The basic suite passes. Once these issues are fixed, CTest should report
`100% tests passed`.

To see every check's result, run the executable directly after building:

```sh
./build/evaluator_tests
./build/evaluator_operator_tests
```

The basic suite reports `18/18 evaluator tests passed`. The current operator
summary is `111/139 evaluator operator checks passed`; the target is `139/139`
after implementing conditional evaluation. A failed check prints `FAIL` and
makes the executable return a nonzero exit status. The checks run in both Debug
and Release builds.

Conditional blocks are expected to return their last evaluated value and to
require boolean conditions. Propagation checks verify the existing operand
error message so the generic unsupported-node error cannot satisfy them.
The three invalid numeric condition checks currently fail with
`std::bad_variant_access` instead of the expected `std::runtime_error` because
the truthiness guard is inverted. Empty branch results and a false `if` without
an `else` are not asserted by the current conditional cases. `Value` now has a
null alternative; add explicit coverage as those behaviors are completed.

The active suite does not test the REPL or parser, floating unary minus, or
boolean/numeric equality rejection.
Division by zero and integer overflow need evaluator guards before adding
in-process rejection tests, since the current operations can crash or invoke
undefined behavior. Declaration, function, and return execution are also outside
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
