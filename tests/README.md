# Interpreter tests

Regression cases for all 13 numbered findings in `error.md` are documented in
[ERROR_REGRESSIONS.md](ERROR_REGRESSIONS.md), with file locations, an error-to-test
mapping, and run commands. They are registered under the `error_regression`
CTest label and intentionally fail for defects that remain unfixed.

Four core test executables exercise the parser and evaluator:

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
- `evaluator_lifetime_tests.cpp` runs 13 checks for retained function bodies and
  captured environments, ancestor scope retention, live binding updates, lexical
  lookup, independent captures, repeated and recursive calls, nested functions,
  environment handle copies, cleanup, and expired-capture diagnostics.
- `parser_statement_tests.cpp` runs 32 checks for rejection of malformed
  declarations, conditionals, and returns at program and block boundaries.
  Invalid statements in unselected branches and uncalled functions must still
  fail parsing. Positive cases preserve empty programs and blocks, uninitialized
  declarations, and valid statement execution.

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
cmake --build build -j2
ctest --test-dir build -LE excluded_error_04 --output-on-failure
```

The four core executables contain 313 checks. Build again after changing the parser, evaluator,
or tests.

The numbered findings have an additional [regression suite](ERROR_REGRESSIONS.md).
Run the requested fixes with:

```sh
ctest --test-dir build -L error_regression --output-on-failure
```

Its 34 labeled cases pass. The original error 4 assertions remain registered
under `excluded_error_04`, outside this requested fix and both commands above.

The basic and operator executables build. All 18 basic checks and all 250 operator checks pass
in Debug and UndefinedBehaviorSanitizer builds. All 19 return checks pass,
including the zero-argument function calls and rejection of returns outside
functions. These return checks do not yet establish support for function parameters,
local call scopes, or return-type validation.

To see every check's result, run the executable directly after building:

```sh
./build/evaluator_tests
./build/evaluator_operator_tests
./build/evaluator_lifetime_tests
./build/parser_statement_tests
```

The basic suite reports `18/18 evaluator tests passed`. The current operator
summary is `250/250 evaluator operator checks passed`. A failed check prints `FAIL` and
makes the executable return a nonzero exit status. The checks run in both Debug
and Release builds.

The lifetime suite reports `13/13 evaluator lifetime checks passed`. It destroys
ASTs and stack environment handles before calling retained functions, and checks
that runtime teardown releases captured scopes without an ownership cycle. If a
function survives every owner of its defining scope, calling it must report
`Function environment has expired`. A surviving environment handle can still
keep that scope available after the original evaluator is destroyed.
Nested function escape is exercised through the body execution API because
source function return types currently exclude function values.
The lifetime suite also passes with AddressSanitizer, UndefinedBehaviorSanitizer,
and LeakSanitizer enabled.

The statement suite reports `32/32 parser statement checks passed`. Rejection
cases call `ParseProgram()` without evaluating its output, so a runtime error
during evaluation cannot conceal a parser accepting invalid source. Integration
checks also verify that parsing failure prevents execution of earlier statements
and that existing bindings remain usable. This suite does not define general
parser recovery or REPL exception handling.

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

The four core suites do not test the REPL, parser validation beyond statement failures, floating
unary minus, or boolean/numeric equality rejection. Division-by-zero and signed
integer overflow checks require the evaluator's guards to throw before the
underlying arithmetic executes. The lifetime suite covers bindings, parameters,
and recursion in retained-function cases; broader binding validation, missing
returns, and return-type mismatch checks remain outside the active suite.

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
