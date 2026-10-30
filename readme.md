Following Writing an Interpreter in Go book by Thorsten Bell except in c++

 
Source -> Lexer -> Parser -> AST -> Evaluator -> RunTime Values  

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --target evaluator_tests
./build/evaluator_tests

All tests were written by codex

tests written in the test folder
ctest --test-dir build -R '^evaluator_operator_tests$' --output-on-failure

Function bodies use shared ownership between the AST and runtime function values.
Each `Environment` is a handle to a heap frame; copying a handle shares that scope.
Frames keep their parent scopes alive. The `Evaluator` retains frames captured by
functions until the evaluator is destroyed, so captures survive the destruction
of local `Environment` handles and parsed programs. Calls create fresh child frames.

Functions hold weak references to defining frames to avoid ownership cycles.
Calling a function after its defining frame has expired throws
`Function environment has expired`. An external `Environment` handle can also
keep a frame alive after the original evaluator is destroyed. Captured frames
are retained for the evaluator's lifetime; this implementation does not collect
unused captured scopes during a session.
