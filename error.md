Project review of the current working tree, including existing uncommitted changes. Reviewed `main.cpp`, `lexxer.hpp`, `parser.hpp`, `ast.hpp`, `token.hpp`, the empty `object.hpp`, `CMakeLists.txt`, and `readme.md`. No implementation files were changed for this review.

The Debug CMake build succeeds with GCC 15.2.1. CTest reports **“No tests were found”**; the assertions in `main.cpp` are commented out. Findings below were checked using the executable, a temporary AST/token inspection harness, UndefinedBehaviorSanitizer, standard-library assertions, or compiler probes. Arithmetic precedence, grouping, consecutive initialized declarations, and calls with zero or two arguments worked in the exercised cases.

Inputs below are source text passed to `Parser::ParseProgram()` unless CLI behavior is specified. **High** means a crash or corruption of otherwise valid input; **medium** means incorrect parsing, lost information, or an API/build failure under the stated conditions.

Error.md produced by codex, I will still correct the errors myself.



6. **Medium — Unterminated blocks are accepted at EOF.**

   Location: `parser.hpp:330–347`.

   `if (1) { int x = 2;` and `int f() { return 1;` both return ASTs without reporting the missing `}`. The block loop treats EOF and a closing brace as equivalent successful endings.

   Suggested fix: require `}` after the loop and return a parse error when EOF arrives first.

7. **Medium — Declaration/return terminators are consumed without validation.**

   Locations: `parser.hpp:274–278`, `parser.hpp:323–326`.

   Both parsers unconditionally call `nextToken()` after their expression, assuming the following token is a semicolon. `int x = 1 int y = 2;` silently consumes the second `int`. `return 1 return 2;` drops the second `return` keyword.

   For `int f() { return 1 } int y = 2;`, this consumes the closing brace and incorrectly puts `y` inside the function body. Whether semicolons are required or optional, the parser should not silently eat unrelated tokens.

   Suggested fix: require and consume `;` if the language requires it; otherwise consume it only when present and preserve block delimiters.

8. **Medium — Function declarations lose their return type.**

   Locations: `parser.hpp:453–459`, `parser.hpp:494–508`, `ast.hpp:271–286`.

   `parseTypedStatement()` saves the type token, but calls `parseFunctionLit()` after advancing to the function name and does not pass the type. The function's only token is therefore its name, and the AST has no separate return-type field.

   For `double f(int x) { return x; }`, the function token is `f`; the fact that it returns `double` is absent from the AST.

   Suggested fix: pass the saved type into the function parser and store it separately from the name.

9. **Medium — Whitespace generates illegal tokens.**

   Locations: `lexxer.hpp:37–43`, `lexxer.hpp:114–128`, `lexxer.hpp:166–173`.

   Only the literal space character is skipped. Tabs, newlines, and carriage returns are emitted as `ILLEGAL`, so `int\tx = 1;` fails to produce the intended declaration. Passing `int x = 1;\nint y = 2;` directly to the parser creates an extra invalid statement between declarations. Here `\t` and `\n` denote actual tab/newline characters.

   There is also no EOF check after skipping spaces. An input consisting of one space produces `ILLEGAL` with empty text before EOF; `int x = 1; ` produces an extra invalid AST statement. This is an incorrect token result, not evidence of an out-of-bounds string read.

   Suggested fix: skip all supported whitespace with bounds checks, then check for EOF before classifying another character.

10. **Medium — Out-of-range integer literals silently become zero.**

    Location: `parser.hpp:99–105`.

    The result of `std::from_chars()` is ignored. When the input exceeds the range of `int`, conversion fails and the initially zero value is stored in the AST.

    `int x = 9999999999999999999999999999;` prints `0` without an error. On the tested platform, `int x = -2147483648;` becomes a unary minus applied to zero, since the positive magnitude is parsed into an `int` before negation.

    Suggested fix: check the conversion error and consumed range, report overflow, and decide how signed boundary literals should be represented.

11. **Medium — Printing an empty program accesses an empty vector.**

    Location: `ast.hpp:48–49`.

    `Program::print()` unconditionally calls `statements.front()`, although `ParseProgram()` can return an empty program. Calling `print()` on a parsed empty string triggers the standard-library assertion that the vector must not be empty. Without that check, the access has undefined behavior.

    The CLI's explicit empty-program check avoids this particular access, but direct users of `Program::print()` remain affected.

    Suggested fix: handle an empty program before accessing its statements.

12. **Medium — Public headers fail when included more than once.**

    Locations: `ast.hpp:1`, `parser.hpp:1`.

    Neither header has an include guard or `#pragma once`. Compiler probes fail with class redefinition errors when including `ast.hpp` twice, including `parser.hpp` twice, or including `parser.hpp` followed by `ast.hpp` in one translation unit. The current executable happens not to use those include combinations.

    Suggested fix: add an include guard to both headers.

13. **Medium — Normal input errors terminate the CLI with uncaught exceptions.**

    Locations: `main.cpp:99–125`, `parser.hpp:469–479`.

    Empty input/EOF, `int f(int) { return 1; }`, and `int f() return 1;` throw `std::runtime_error` that escapes `main()`. Each reproduced case terminates through `SIGABRT` rather than returning a controlled diagnostic and exit status.

    Suggested fix: distinguish EOF from malformed input, catch/report parser errors at the CLI boundary, and return an ordinary nonzero exit status for invalid source.

The following behavior is also confirmed, but may reflect unfinished features rather than unintended regressions:

- **Output omits most of the program.** `main.cpp:113–115` prints only the first statement, as does `Program::print()` at `ast.hpp:48–49`. The print methods for blocks, conditionals, function parameters, functions, calls, and expression statements are empty (`ast.hpp:231`, `252`, `267`, `288`, `307`, `326`). Thus `int x = 1; int y = 2;` displays only `x`; `1 + 2;`, `f(1, 2);`, and a function declaration display only the input prompt. Implementing recursive printing and iterating over all program statements would make successful parsing observable.
- **`double` declarations cannot represent decimal literals.** `lexxer.hpp:63–71` only reads integers, `.` is illegal, and `parser.hpp:192–214` has no floating-point literal branch. `double x = 1.5;` becomes a declaration initialized with integer `1` plus a separate expression `5`, without a diagnostic. `TokenType::FLOAT` exists at `token.hpp:32` but is neither produced by the lexer nor handled by `Convert_type_to_str()`; the latter also causes a compiler warning. Implement decimal literals or explicitly reject them until supported.
- **Boolean literals are parsed as identifiers.** `true` and `false` are absent from the keyword map at `lexxer.hpp:15–22`, and expression parsing never constructs the `Boolean` node defined at `ast.hpp:123–140`. `bool x = true;` stores an `Identifier("true")`, not a boolean literal. If boolean literals are intended, add their tokenization and parsing.
- **CLI input is limited to one line.** `main.cpp:101` calls `std::getline()` once and `main()` does not loop. Piping two lines parses only the first. This may be an intentional debugging interface, but multiline source and repeated interactive input are currently unsupported.

Validation can be repeated from the project root with an isolated build:

```sh
cmake -S . -B /tmp/compiler-error-check -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/compiler-error-check -j2
ctest --test-dir /tmp/compiler-error-check --output-on-failure

g++ -std=c++20 -g -fsanitize=undefined -fno-sanitize-recover=undefined \
    -D_GLIBCXX_ASSERTIONS main.cpp -o /tmp/compiler-error-check/lexer-ubsan
printf 'int x = ;\n' | /tmp/compiler-error-check/lexer-ubsan
printf 'int x = 9999999999999999999999999999;\n' | /tmp/compiler-error-check/lexer

printf '#include "parser.hpp"\n#include "ast.hpp"\n' | \
    g++ -std=c++20 -I . -x c++ -fsyntax-only -
```

The sanitizer example and repeated-header compilation are expected to fail and demonstrate the reported defects. AST-only findings were verified by traversing the returned nodes directly, since the existing print methods cannot expose them reliably. This was a focused review of the current parser/lexer; it does not establish that all other inputs are correct.
