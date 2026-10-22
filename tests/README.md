# Evaluator tests

Two executables test the current `evaluator.hpp`:

- `evaluator_tests.cpp` runs 18 checks for integer and boolean literals,
  expression statements, statement lists, programs, and unsupported nodes.
- `evaluator_operator_tests.cpp` runs 250 checks for float literals, recursive
  prefix/infix evaluation, all seven infix operators, integer division,
  float/double arithmetic, mixed numeric operands, boolean equality, numeric
  operand validation, error propagation, and reuse after an error. This includes
  23 new conditional checks for branch selection, computed boolean conditions,
  skipped branches, nested conditionals, branch result types, multi-statement
  blocks, invalid condition types, and continued program execution. Another 19
  checks cover return values, early exits, nested returns, skipped statements,
  error propagation, and rejection of returns outside functions.
  Another 92 checks cover checked integer arithmetic, exact bounds, multiplication
  signs and zero operands, division truncation, zero divisors, and preservation
  of float/double/mixed arithmetic beyond integer bounds. Six of these parse
  and evaluate the source examples from finding 2 in `error.md`.

The tests check runtime types as well as values. Float and double results allow
a small rounding tolerance. Most AST nodes are constructed directly to keep
evaluation tests independent from the lexer and parser; six arithmetic
regressions also exercise the source-to-value path. Double operands are passed to
`evalInfixExpression()` directly because there is no double-literal AST node.
Calls to `eval()`, `execute()`, and `evalStatement()` pass an explicit
`Environment&`. Shared evaluator cases reuse the same environment; isolated
function and outside-function return cases create a fresh environment.
Numeric-pair checks exercise addition directly, including mixed result types
and rejection of boolean operands, instead of calling a numeric-check helper.
Failure diagnostics also handle function values in `Value`.

From the project root (`/home/riccardo/compiler`), run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --target evaluator_tests evaluator_operator_tests
ctest --test-dir build -R '^evaluator(_operator)?_tests$' --output-on-failure
```

The two executables contain 268 checks. Build again after changing the evaluator
or tests.

Both test executables build. All 18 basic checks and all 250 operator checks pass
in Debug and UndefinedBehaviorSanitizer builds. All 19 return checks pass,
including the zero-argument function calls and rejection of returns outside
functions. These checks do not yet establish support for function parameters,
local call scopes, or return-type validation.

To see every check's result, run the executable directly after building:

```sh
./build/evaluator_tests
./build/evaluator_operator_tests
```

The basic suite reports `18/18 evaluator tests passed`. The current operator
summary is `250/250 evaluator operator checks passed`. A failed check prints `FAIL` and
makes the executable return a nonzero exit status. The checks run in both Debug
and Release builds.

Conditional blocks are expected to return their last evaluated value and to
require boolean conditions. Direct conditional checks call `execute()` and
verify that ordinary execution has no return signal before comparing values.
Invalid-condition and propagation checks verify the existing diagnostics so a
generic unsupported-node error cannot satisfy them.
Empty branch results and a false `if` without an `else` are not asserted by the
current conditional cases. `Value` has a null alternative; add explicit coverage
as those behaviors are completed.

Return tests follow `todo.md`: `return` is only valid inside a function. Success
cases declare and call a zero-argument `int` or `bool` function using AST nodes
and a fresh evaluator and environment per case. They check that returning zero
or false still stops execution, returns escape nested blocks and conditionals,
unselected returns have no effect, and a function return does not terminate its
caller.
Outside-function cases execute their statements through a `Program`; their
errors must contain `Return should be in function`. Operand-error cases preserve
the existing operand diagnostic. This prevents the current generic unsupported
AST error from incorrectly satisfying these error checks.

The active suite does not test the REPL, general parser validation, floating
unary minus, or boolean/numeric equality rejection. Division-by-zero and signed
integer overflow checks require the evaluator's guards to throw before the
underlying arithmetic executes. General bindings, function parameters, recursion,
missing returns, and return-type mismatch checks remain outside the active suite.

## Planned language rule tests

`language_rules_tests.hpp` contains 65 cases for section 1 of `todo.md`, including
arithmetic, runtime type checks, overflow, bindings, and function calls. This
broader suite is not registered with CTest. It still needs an interpreter
adapter and the remaining language and scope behavior before it can pass against
the interpreter. The active operator suite covers integer arithmetic rejection
independently.

Later, connect it with a function passed to `language_rules_tests::run()`. That
function accepts a source string, executes it in a fresh environment, and
returns the last expression's value or an expected runtime error category.
Unexpected parsing failures must remain test failures. All source inputs use
ordinary spaces.
