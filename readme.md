Following Writing an Interpreter in Go book by Thorsten Bell except in c++

 
Source -> Lexer -> Parser -> AST -> Evaluator -> RunTime Values  

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --target evaluator_tests
./build/evaluator_tests

All tests were written by codex

tests written in the test folder
ctest --test-dir build -R '^evaluator_operator_tests$' --output-on-failure
