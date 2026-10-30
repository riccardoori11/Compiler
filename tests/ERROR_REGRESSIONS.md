# Regression tests for error.md

These tests cover all 13 numbered findings in `../error.md`. They assert the
intended corrected behavior: failing cases indicate defects, not expected
successes. Error 4 is explicitly excluded from the requested fix and from the
`error_regression` label; its tests remain registered under `excluded_error_04`
with their original assertions. The unnumbered language-design gaps are outside
this suite.

Each case runs in a separate process with a timeout, so crashes cannot stop
the suite. Error 13 builds its separate public API caller during CTest, keeping
its linker failure out of the normal build. Do not run another build concurrently
with that test in the same build directory.

Verified on 2026-10-30 with the repository's GCC Debug build: **34/34 labeled
cases passed**, covering findings 1–3 and 5–13, the three controls, and two
additional integration cases. All four pre-existing CTest suites passed too.
All 38 in-scope CTest cases also pass in a separate AddressSanitizer,
UndefinedBehaviorSanitizer, and LeakSanitizer build.

| Error | CTest prefix | File and coverage |
| --- | --- | --- |
| 1 | `error_01_` | `error_regression_tests.cpp`: reject malformed grouped operand during parsing |
| 2 | `error_02_` | `error_regression_tests.cpp`: all six arithmetic reproductions must throw |
| 3 | `error_03_` | `error_regression_tests.cpp`: independently destroy AST/environment before calling; `evaluator_lifetime_tests.cpp` adds teardown and broader capture coverage |
| 4 | `error_04_` | Excluded: original printing assertions remain in `error_regression_tests.cpp` under `excluded_error_04` |
| 5 | `error_05_` | `error_regression_tests.cpp`: reject semicolon and illegal argument separators |
| 6 | `error_06_` | `error_regression_tests.cpp`: reject malformed declaration/conditional before evaluation; `parser_statement_tests.cpp` adds broader coverage |
| 7 | `error_07_` | `error_regression_tests.cpp`: reject leaked name; allow shadowing without changing outer binding |
| 8 | `error_08_` | `error_repl_test.cmake`: actual REPL preserves variables and functions across lines |
| 9 | `error_09_` | `error_repl_test.cmake`: actual REPL executes `42;` after runtime error, parse error, or blank input |
| 10 | `error_10_` | `error_regression_tests.cpp`: reject oversized integer/float literals; minimum integer must be exact or explicitly rejected, never silently zero |
| 11 | `error_11_` | `error_regression_tests.cpp`: tabs, LF, CR, CRLF work as whitespace |
| 12 | `error_12_` | `error_regression_tests.cpp`: non-callable value throws interpreter `runtime_error`, not `bad_variant_access` |
| 13 | `error_13_` | `error_type_link_probe.cpp` + `error_type_link_test.cmake`: separate caller links with `object.cpp` and returns `int` for `type(Value{1})` |

Three `error_control_` cases cover valid calls, integer bounds, and normal REPL
input/EOF. `error_extended_contracts` adds 19 checks for grouped failures, nested
scopes and returns, parameter scope, numeric boundaries, whitespace, callable
diagnostics, and all runtime type names. `error_extended_repl_recovery` checks
that variables and functions survive multiple invalid and blank input lines.
There are **36 CTest cases** in this suite: 34 labeled `error_regression` and
the two excluded error 4 cases. Registration is in `error_regressions.cmake`,
included by the root CMake file when `BUILD_TESTING` is enabled. Numeric examples
require the report's 32-bit `int`.

From the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j2
ctest --test-dir build -L error_regression --output-on-failure
```

Run the excluded error 4 cases explicitly (these remain known failures):

```sh
ctest --test-dir build -R '^error_04_' --output-on-failure
```

For lifetime and undefined-behavior verification, use a separate build:

```sh
cmake -S . -B /tmp/compiler-error-sanitizers \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -D_GLIBCXX_ASSERTIONS'
cmake --build /tmp/compiler-error-sanitizers -j2
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 \
  ctest --test-dir /tmp/compiler-error-sanitizers -LE excluded_error_04 --output-on-failure
```

LeakSanitizer requires an environment without process tracing. Infrastructure
failures do not establish interpreter defects. Detailed CTest results are saved
in `Testing/Temporary/LastTest.log` inside the selected build directory.
