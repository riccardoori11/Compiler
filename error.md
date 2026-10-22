# Current code review — 2026-10-22

Reviewed the current lexer, parser, AST, evaluator, environment, runtime objects,
REPL, and test setup. Only this report was changed; implementation fixes are
left to the author.

A fresh Debug build with GCC 15.2.1 succeeds. Both registered CTest suites pass:
**18/18 basic evaluator checks and 158/158 operator checks**. The findings below
come from additional source-level probes, direct API calls, AddressSanitizer,
UndefinedBehaviorSanitizer, standard-library assertions, and a linker probe.
Passing the existing suites does not cover these cases.

Update: finding 2 is now fixed. The expanded suites pass **18/18 basic checks
and 250/250 operator checks** in both Debug and UndefinedBehaviorSanitizer builds.

This replaces the older review, whose build status and some findings were
outdated. Include guards, boolean literal parsing, decimal tokenization, function
return types, and the repeated-input loop are now implemented. Plain EOF exits
normally. Integer conversion errors, unsafe AST printing, and uncaught input
errors remain and are documented below with current locations.

**High** means a crash, invalid memory access, or undefined arithmetic.
**Medium** means accepted invalid source, incorrect scope/state, uncontrolled
error handling, or an unusable public API. Locations refer to this working tree.

1. **High — A failed grouped expression can become an AST with a null operand.**

   Locations: `parser.hpp:143–159, 268–284, 343–357`;
   `evaluator.hpp:351–356`.

   Reproduction:

   ```text
   (1 + ) + 2;
   ```

   `ParseGroupedExpression()` returns null after the invalid inner addition.
   `ParseExpression()` nevertheless continues extending that null result with
   the following `+ 2`, producing an `InfixExpression` whose left child is null.
   Parsing succeeds with one statement. Evaluation dereferences the missing
   operand; the standard-library assertion in `unique_ptr::operator*` aborts.

   Suggested fix: stop expression parsing immediately when a prefix/grouped
   parse fails, and require both operands before constructing an infix node.
   Apply the same validation before constructing a call from a failed callee.

2. **Resolved — Integer division and arithmetic reject invalid operations.**

   Implementation: `validateIntegerInfix()` and the unary-minus case in
   `evaluator.hpp`. Regression tests: `check_integer_arithmetic()` in
   `tests/evaluator_operator_tests.cpp`.

   The original UBSan reproductions on the current 32-bit `int` platform now
   throw interpreter runtime errors:

   | Source | Runtime error |
   |---|---|
   | `1 / 0;` | `Can't divide by zero` |
   | `2147483647 + 1;` | `Integer overflow` |
   | `(-2147483647 - 1) - 1;` | `Integer overflow` |
   | `2147483647 * 2;` | `Integer overflow` |
   | `-(-2147483647 - 1);` | `Integer overflow` |
   | `(-2147483647 - 1) / -1;` | `Integer overflow` |

   Integer bounds are checked before addition, subtraction, multiplication,
   division by `-1`, and unary negation. The existing zero-divisor check and
   floating-point/mixed arithmetic behavior are preserved.

   Added 92 regression checks covering the six source examples, valid boundary
   values, multiplication signs and zero operands, division truncation, and
   floating-point/mixed operands. All 268 checks pass in isolated Debug and UBSan
   builds, with no undefined-behavior diagnostics. REPL exception recovery remains
   a separate issue under finding 9.

3. **High — A stored function can outlive its AST or defining environment.**

   Locations: `object.hpp:26–31`; `evaluator.hpp:84–103, 274, 291`.

   `shared_ptr<FunctionObj>` keeps only the function object alive. Its `Body` and
   `env` fields are non-owning raw pointers.

   AST reproduction through the evaluator API:

   ```cpp
   Evaluator evaluator;
   Environment env;
   {
       Parser parser(lexxer{"int f() { return 7; }"});
       auto program = parser.ParseProgram();
       evaluator.eval(*program, env);
   } // program and f's body are destroyed; env still contains f
   Parser parser(lexxer{"f();"});
   auto program = parser.ParseProgram();
   evaluator.eval(*program, env); // accesses the destroyed body
   ```

   This crashes under ASan during dispatch on the freed function body. A second
   direct API probe retained a function value after destroying its defining
   environment, kept its AST alive, and then called it. ASan reports
   `stack-use-after-scope` when its environment is read.

   These are API lifetime bugs. The current REPL resets its environment every
   line, so it does not currently preserve a function long enough to reproduce
   the first case across lines. Merely moving that environment outside the loop
   would expose it unless AST retention is fixed too.

   Suggested fix: define ownership for retained ASTs and captured environments
   before enabling persistent functions or escaping closures.

4. **High — AST printing crashes on valid uninitialized declarations and empty programs.**

   Locations: `ast.hpp:50–51, 95–99`.

   Direct API reproductions:

   - Parse `int x;`, then call `program->print()`: the declaration dereferences
     its null initializer. UBSan reports member access through a null pointer.
   - Parse an empty string, then call `program->print()`: `front()` accesses an
     empty vector and triggers a standard-library assertion.

   The current REPL does not call these print methods, but both are public AST
   operations on nodes the parser can legitimately produce.

   Suggested fix: handle missing initializers and empty statement lists before
   dereferencing them.

5. **Medium — The first call-argument separator is never checked.**

   Location: `parser.hpp:179–190`.

   Both inputs below are incorrectly accepted and evaluate to `3`:

   ```text
   int f(int a, int b) { return a + b; } f(1; 2);
   int f(int a, int b) { return a + b; } f(1 @ 2);
   ```

   After parsing the first argument, the parser advances twice without first
   requiring a comma. The semicolon or illegal `@` token is silently skipped.

   Suggested fix: require `COMMA` before advancing to each additional argument,
   including the second argument.

6. **Medium — Failed statements can be discarded while the remaining input executes.**

   Locations: `parser.hpp:310–320, 428–430, 408–414, 650–658`.

   Confirmed examples:

   | Invalid source | Actual result |
   |---|---|
   | `int x 1;` | Evaluates to `1`; the malformed declaration disappears |
   | `if true;` | Evaluates to `true`; the malformed `if` disappears |

   Some statement parsers signal failure with null. The program/block loops
   simply omit that result and keep parsing, rather than rejecting the source.

   Suggested fix: propagate a parse error for a failed statement. An intentionally
   empty construct needs a distinct representation from failed parsing.

7. **Medium — Blocks share their caller's bindings, leaking names and preventing shadowing.**

   Location: `evaluator.hpp:127–131`.

   ```text
   if (true) { int x = 7; } x;
   ```

   This returns `7`; under block scoping, `x` should be undefined outside the
   selected block.

   ```text
   int x = 1; if (true) { int x = 2; } x;
   ```

   This throws the duplicate-name error instead of allowing the inner declaration
   and leaving the outer `x` equal to `1`.

   Cause: block execution passes the original environment directly to its
   statements. This also means declarations in a function-body conditional
   remain visible after that conditional.

   Suggested fix: execute lexical blocks in a child environment, preserve return
   propagation, and explicitly decide whether a function body's outermost block
   shares its parameter scope.

8. **Medium — REPL bindings are discarded after every input line.**

   Location: `main.cpp:1217–1224`.

   Enter these as two separate lines:

   ```text
   int x = 1;
   x;
   ```

   The first prints `null`; the second reports `could not find name` and aborts.
   `Environment e{}` is constructed inside the loop. Functions declared on a
   previous line are likewise unavailable.

   Suggested fix: retain a global environment across inputs. Also address finding
   3 so functions retain valid bodies after each parsed program is destroyed.

9. **Medium — Recoverable input errors terminate the REPL.**

   Location: `main.cpp:1217–1224`.

   Piping either input below aborts through an uncaught exception and never
   evaluates the following `42;`:

   ```text
   !1;
   42;
   ```

   A blank line followed by `42;` also aborts, with `Empty statements`. Normal
   EOF works; the defect is the handling of invalid or empty input within the
   loop.

   Suggested fix: catch parser/runtime failures per input, report a diagnostic,
   and continue reading. Skip blank input or define its result explicitly.

10. **Medium — Out-of-range numeric literals silently evaluate to zero.**

    Locations: `parser.hpp:112–118, 220–226`.

    Both integer and float parsing ignore the result of `std::from_chars()`.
    Confirmed examples:

    ```text
    9999999999999999999999999999;
    9999999999999999999999999999999999999999999999999999.0;
    ```

    Both evaluate to `0` of their respective runtime types. On this platform,
    `-2147483648;` also evaluates to integer zero: parsing its positive magnitude
    fails before unary negation runs.

    Suggested fix: check the conversion error and consumed range for both kinds
    of literal, and define how the minimum signed integer is parsed.

11. **Medium — Tabs and line breaks are illegal tokens instead of whitespace.**

    Location: `lexxer.hpp:39–44`.

    The lexer skips only the literal space character. `int\tx = 1; x;`, where
    `\t` is an actual tab, fails to parse. Passing `int x = 1;\nx;` directly to
    the parser, with an actual line break, fails too. Carriage returns have the
    same problem, including those left by CRLF input read with `getline()`.

    Suggested fix: recognize the intended whitespace characters with bounds
    checks. If using `std::isspace`, convert the character to `unsigned char`.

12. **Medium — Calling a non-function exposes a C++ variant exception.**

    Location: `evaluator.hpp:260–266`.

    ```text
    int x = 1; x();
    ```

    `std::get<shared_ptr<FunctionObj>>()` throws `std::bad_variant_access` before
    the null-pointer check. Direct evaluator users receive a C++ storage error
    rather than the interpreter's runtime diagnostic; the REPL aborts.

    Suggested fix: check the held alternative, or use `std::get_if`, and report
    that the value is not callable before accessing function fields.

13. **Medium — The public `type()` helper cannot link from a normal caller.**

    Locations: `object.hpp:53`; `object.cpp:30–49`.

    The abbreviated function template is declared in the header but defined only
    in `object.cpp`, with no explicit instantiation. This caller compiles but
    fails to link even when linked with `object.cpp`:

    ```cpp
    #include "object.hpp"
    #include <iostream>
    int main() {
        Value value{1};
        std::cout << type(value);
    }
    ```

    The linker reports an undefined reference to `type<std::variant<...>>()`.
    The main executable does not currently call this helper, so its successful
    build does not exercise the defect.

    Suggested fix: use a non-template `type(const Value&)`, put the template
    definition in the header, or explicitly instantiate the supported types.

Additional incomplete behavior, separate from the confirmed failures above:

- Decimal literals always produce `float`, while the parser exposes `double`
  declarations and parameters. `double x = 1.5; x;` fails with `Wrong type
  mismatch`; there is no source literal producing a `double`. Define conversion
  rules or add a matching literal representation.
- `-1.5;` fails with `Invalid right expression for MINUS`, although positive
  floats and floating infix arithmetic work. Unary numeric support is incomplete.
- `Environment::Set()` searches only the local map even though `Get()` searches
  parents. A direct child-environment update cannot reach an outer binding.
  Source-level assignment is not implemented, so this is currently an API/design
  gap rather than a failing assignment expression.
- `if` is currently a statement, so `int x = if (true) { 9; };` is unsupported.
  This requires a language-design decision, rather than a parser bug by itself.

Validation used an isolated build, leaving the repository's build outputs alone:

```sh
cmake -S . -B /tmp/compiler-bug-review -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build /tmp/compiler-bug-review -j2
ctest --test-dir /tmp/compiler-bug-review --output-on-failure
```

A temporary harness in `/tmp/compiler-review-probe.cpp` parsed and evaluated
individual inputs, exercised printing and lifetime cases, and caught C++
exceptions. It was compiled with:

```sh
g++ -std=c++20 -g -O0 -fsanitize=address,undefined \
    -fno-sanitize-recover=all -D_GLIBCXX_ASSERTIONS -I . \
    /tmp/compiler-review-probe.cpp object.cpp -o /tmp/compiler-review-probe
```

The temporary harness and detailed probe output (`/tmp/compiler-review-results.txt`)
are local review artifacts, not permanent project tests. Each crashing example
was run in a separate process. Function arguments (`add(2, 3)` → `5`), recursion
(`factorial(5)` → `120`), and the existing 176 checks also succeeded as controls.
