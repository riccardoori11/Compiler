# Evaluator TODO

Build a tree-walking interpreter using the existing AST. Start with the current
`Value = std::variant<int, bool>` in `object.hpp`, then add functions and doubles.

```text
source -> lexer -> parser -> AST -> evaluator -> runtime values
                                      |
                                 environment
```

**1. Define the initial language rules**

- [ ] Start with integer and boolean values. Report unsupported types, including
  `double`, until their evaluation is implemented.
- [ ] Define operator rules: integer arithmetic and ordering; boolean `!`;
  equality between values of the same type. Require boolean `if` conditions.
- [ ] Use integer division for two integers, truncating toward zero. Report
  division by zero and integer overflow as runtime errors.
- [ ] Require initializer, argument, and return values to match their declared
  types for the initial implementation.
- [ ] Treat `int x;` as an uninitialized binding and report an error when it is
  read before initialization. Reject duplicate declarations in the same scope;
  allow inner scopes to shadow outer bindings.

**2. Finish runtime values (`object.hpp`)**

- [ ] Add an include guard or `#pragma once` to `object.hpp`, `ast.hpp`, and
  `parser.hpp` so the evaluator can safely include them together.
- [ ] Finish the incomplete declaration in `object.hpp` by implementing
  `std::string inspect(const Value&)` with `std::visit`.
- [ ] Format integers as decimal text and booleans as `true` or `false`. Return
  an owning `std::string` so generated text remains valid after the call.
- [ ] Add a helper for readable runtime type names in diagnostics.
- [ ] Keep runtime values separate from AST ownership: evaluating an integer
  literal should produce its integer value without moving or changing the node.

**3. Complete literal parsing needed by evaluation**

- [ ] Add tokens and keyword entries for `true` and `false` in `token.hpp` and
  `lexxer.hpp`.
- [ ] Add boolean literal parsing in `parser.hpp` that constructs `Boolean`
  nodes. Change the `Boolean` constructor's value parameter to `bool`.
- [ ] Check the result of `std::from_chars` in `parseInteger()` and report
  malformed or out-of-range integer literals. Define how the minimum signed
  integer literal is handled when its positive magnitude does not fit in `int`.
- [ ] Ensure failed parsing is reported before evaluation begins; do not run a
  partially parsed program after an input error.

**4. Evaluate expressions (`evaluator.hpp`)**

- [ ] Create an `Evaluator` with `Value evaluate(const Expression&,
  Environment&)`. Start with `dynamic_cast` dispatch for the existing AST
  hierarchy and report unsupported node types explicitly.
- [ ] Evaluate `Integer_Liter` and `Boolean` by returning their stored values.
- [ ] Evaluate `PrefixExpression`: recursively evaluate its operand, check its
  type, and apply unary `-` or `!`.
- [ ] Evaluate `InfixExpression`: evaluate the left operand, then the right
  operand, and dispatch on `token.tokentype`.
- [ ] Implement `+`, `-`, `*`, `/`, `<`, `>`, and `==`. Comparisons produce a
  boolean `Value`; arithmetic produces an integer `Value`.
- [ ] Check operand types and arithmetic bounds before performing operations.
  Include the operator and operand types in runtime error messages.
- [ ] Use a consistent runtime error mechanism, such as a `RuntimeError`
  exception, and handle it at the program/REPL boundary.

First milestone: `2 + 3 * 4` evaluates to `14`, `-(2 + 3)` to `-5`, and
`2 < 3` to `true`. The parser has already encoded precedence in the AST.

**5. Store bindings and execute statements (`environment.hpp`, `evaluator.hpp`)**

- [ ] Create an `Environment` containing a map from names to bindings, with a
  declared type and optional initialized value for each variable.
- [ ] Add an optional parent environment and implement lookup from the current
  scope outward. Report undefined names and uninitialized reads separately.
- [ ] Evaluate `Identifier` by looking up `Identifier::value`.
- [ ] Introduce `ExecutionResult` with an `std::optional<Value> returned` field:
  an empty optional means execution continues; a value means a function return.
- [ ] Add `ExecutionResult execute(const Statement&, Environment&)` and a
  program entry point that executes all `Program::statements` in order.
- [ ] Execute `VariableDeclaration`: handle a missing initializer, otherwise
  evaluate it, validate its type, and define the binding.
- [ ] Execute `ExpressionStatement` by evaluating its `expr`. Define a separate
  REPL path for displaying expression results with `inspect()`.
- [ ] Execute `BlockStatements` in a child environment and propagate any return
  signal immediately, skipping the remaining statements.
- [ ] Execute `IfStatement` by checking its condition and executing only the
  selected branch. Handle a missing `Alternative` and propagate returns.

Second milestone: after `int x = 2 + 3 * 4;`, the environment contains `x = 14`
and evaluating `x + 1` produces `15`.

**6. Add returns and function calls**

- [ ] Execute `Return` by evaluating `Returnvalue` and producing a return
  signal. Reject `return` outside a function.
- [ ] Define `RuntimeFunction` with access to its AST declaration (name, typed
  parameters, return type, and body) and the environment where it was declared.
- [ ] Extend `Value` to represent functions and extend `inspect()` with a
  readable function representation.
- [ ] Choose ownership that keeps function ASTs and captured environments alive
  for as long as they are needed, including across REPL inputs. Account for
  cycles if environments and functions use `shared_ptr` to own one another.
- [ ] Execute `FunctionLiteral` by registering the named function in its
  declaration environment. Run the body when the function is called.
- [ ] Evaluate `CallExpression::function_name`, verify that it is callable, and
  evaluate arguments in the caller's environment from left to right.
- [ ] Check argument count and types. Create a fresh call environment whose
  parent is the function's declaration environment, then bind the parameters.
- [ ] Execute the function body, consume its return signal, validate the return
  type, and return the resulting `Value` to the caller.
- [ ] Report a missing return value when a typed function reaches its end.
- [ ] Support recursive calls. Initially require a function declaration to have
  executed before that function can be called.

Third milestone: `int add(int a, int b) { return a + b; }` followed by
`add(2, 3);` evaluates the call to `5`.

**7. Connect the evaluator and verify behavior**

- [ ] Update the input path in `main.cpp` to parse and execute the complete
  program and display REPL expression results using `inspect()`.
- [ ] Preserve the global environment between REPL inputs and retain ASTs needed
  by previously declared functions. Handle EOF normally and catch/report errors.
- [ ] Add evaluator tests and register them with CTest in `CMakeLists.txt`.
- [ ] Test arithmetic precedence, unary operators, integer division, boolean
  equality, comparisons, and inspection output.
- [ ] Test undefined names, uninitialized reads, duplicate declarations,
  shadowing, and local variables staying inside their scope.
- [ ] Test conditionals, including verifying that the unselected branch is never
  executed and that returns escape nested blocks immediately.
- [ ] Test functions with zero and multiple arguments, recursion, lexical scope,
  wrong argument counts/types, wrong return types, and missing returns.
- [ ] Test type mismatches, division by zero, arithmetic overflow, and continued
  REPL use after a runtime error.

**8. Add double support after the integer/boolean evaluator works**

- [ ] Add decimal tokenization and a floating-point literal AST node/parser path.
- [ ] Extend `Value`, inspection, and expression evaluation to support `double`.
- [ ] Define numeric conversions explicitly, including mixed arithmetic,
  comparisons, typed bindings, arguments, and return values.
- [ ] Add tests for decimal literals, mixed numeric operations, conversions,
  floating-point division by zero, and invalid conversions.
