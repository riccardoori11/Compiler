# Evaluator tests

`evaluator_tests.cpp` runs 18 checks against the current `evaluator.hpp`:
integer and boolean values, expression statements, statement lists, programs,
and unsupported-node errors. Statement list tests check empty input, the last
result's value and type, and errors in the first or middle statement.
It constructs AST nodes directly, so no parser changes or adapter are needed.

From the project root (`/home/riccardo/compiler`), run:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --target evaluator_tests
ctest --test-dir build -R '^evaluator_tests$' --output-on-failure
```

CTest should report `100% tests passed`. It counts the test executable as one
test; that executable performs all 18 checks. Build again after changing the
evaluator or tests.

To see every check's result, run the executable directly after building:

```sh
./build/evaluator_tests
```

Its final line should be `18/18 evaluator tests passed`. A failed check prints
`FAIL` and makes the executable return a nonzero exit status. The checks run in
both Debug and Release builds.

Boolean AST nodes are constructed manually because boolean literal parsing is
not implemented yet. Program tests currently use literal expression statements;
declarations, operators, and return statements still need evaluation support.

## Planned language rule tests

`language_rules_tests.hpp` contains 65 cases for section 1 of `todo.md`, including
arithmetic, runtime type checks, overflow, bindings, and function calls. This
broader suite still needs program evaluation, environments, boolean parsing,
and function execution before it can run against the interpreter.

Later, connect it with a function passed to `language_rules_tests::run()`. That
function accepts a source string, executes it in a fresh environment, and
returns the last expression's value or an expected runtime error category.
Unexpected parsing failures must remain test failures. All source inputs use
ordinary spaces.
