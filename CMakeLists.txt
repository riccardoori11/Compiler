cmake_minimum_required(VERSION 3.16)

project(Lexer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(

		lexer
		main.cpp
		object.cpp
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

    add_executable(evaluator_operator_tests tests/evaluator_operator_tests.cpp)
    target_include_directories(evaluator_operator_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(evaluator_operator_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME evaluator_operator_tests COMMAND evaluator_operator_tests)

    add_executable(evaluator_lifetime_tests tests/evaluator_lifetime_tests.cpp)
    target_include_directories(evaluator_lifetime_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(evaluator_lifetime_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME evaluator_lifetime_tests COMMAND evaluator_lifetime_tests)

    add_executable(parser_statement_tests tests/parser_statement_tests.cpp)
    target_include_directories(parser_statement_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_options(parser_statement_tests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME parser_statement_tests COMMAND parser_statement_tests)

    include(tests/error_regressions.cmake)
endif()

# ---------------------------------------------------------------------------
# ricc: my own standard-library implementations (header-only), used in place
# of std::unique_ptr etc. Pinned to a commit so the compiler doesn't change
# underneath me.
#
# To build against a local checkout instead of GitHub (no push needed):
#   cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_RICC=$HOME/various_implementations
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(ricc
    GIT_REPOSITORY https://github.com/riccardoori11/Various-Implementations.git
    GIT_TAG        c3054a04a6da68ba63413e20b7637dcb8955fb4f # master after PR #12 (ricc::variant operator==)
)
FetchContent_MakeAvailable(ricc)

foreach(target
        lexer
        evaluator_tests evaluator_operator_tests evaluator_lifetime_tests
        parser_statement_tests
        error_regression_tests error_type_link_probe error_extended_tests)
    if(TARGET ${target})
        target_link_libraries(${target} PRIVATE ricc::implementation)
    endif()
endforeach()
