cmake_minimum_required(VERSION 3.16)

project(Lexer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(

		lexer
		main.cpp
)


target_compile_options(lexer PRIVATE
    -Wall
    -Wextra
    -Wpedantic

    $<$<CONFIG:Debug>:-g>
    $<$<CONFIG:Debug>:-O0>
)

include(CTest)

if(BUILD_TESTING)
    add_executable(evaluator_tests tests/evaluator_tests.cpp)
    target_include_directories(evaluator_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(evaluator_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME evaluator_tests COMMAND evaluator_tests)
endif()
