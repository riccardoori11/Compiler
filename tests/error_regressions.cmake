add_executable(error_regression_tests tests/error_regression_tests.cpp)
target_include_directories(error_regression_tests PRIVATE ${PROJECT_SOURCE_DIR})
target_compile_options(error_regression_tests PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(error_regression_tests PRIVATE _GLIBCXX_ASSERTIONS)

set(error_api_cases
    error_01_grouped_operand
    error_02_divide_zero error_02_add_overflow error_02_subtract_overflow
    error_02_multiply_overflow error_02_negate_minimum error_02_divide_overflow
    error_03_destroyed_ast error_03_destroyed_environment
    error_04_print_uninitialized error_04_print_empty
    error_05_semicolon_separator error_05_illegal_separator
    error_06_discarded_declaration error_06_discarded_if
    error_07_block_leak error_07_shadowing
    error_10_integer_range error_10_float_range error_10_minimum_literal
    error_11_tab error_11_newline error_11_carriage_return error_11_crlf
    error_12_non_callable
    error_control_valid_call error_control_integer_bounds)
foreach(case IN LISTS error_api_cases)
    add_test(NAME ${case} COMMAND error_regression_tests ${case})
    if(case MATCHES "^error_04_")
        # Error 4 is explicitly outside the requested fix. Keep its original
        # tests available without including them in the requested regression run.
        set_tests_properties(${case} PROPERTIES LABELS excluded_error_04 TIMEOUT 10)
    else()
        set_tests_properties(${case} PROPERTIES LABELS error_regression TIMEOUT 10)
    endif()
endforeach()

foreach(case IN ITEMS error_08_variable_persistence error_08_function_persistence
        error_09_runtime_recovery error_09_parse_recovery error_09_blank_recovery
        error_control_repl_eof error_extended_repl_recovery)
    add_test(NAME ${case} COMMAND ${CMAKE_COMMAND}
        "-DREPL=$<TARGET_FILE:lexer>" "-DCASE=${case}"
        -P ${PROJECT_SOURCE_DIR}/tests/error_repl_test.cmake)
    set_tests_properties(${case} PROPERTIES LABELS error_regression TIMEOUT 15)
endforeach()

add_executable(error_type_link_probe EXCLUDE_FROM_ALL
    tests/error_type_link_probe.cpp object.cpp)
target_include_directories(error_type_link_probe PRIVATE ${PROJECT_SOURCE_DIR})
add_test(NAME error_13_public_type_link COMMAND ${CMAKE_COMMAND}
    "-DBUILD_DIR=${PROJECT_BINARY_DIR}" "-DCONFIG=$<CONFIG>"
    "-DPROBE=$<TARGET_FILE:error_type_link_probe>"
    -P ${PROJECT_SOURCE_DIR}/tests/error_type_link_test.cmake)
set_tests_properties(error_13_public_type_link PROPERTIES
    LABELS error_regression TIMEOUT 100 RUN_SERIAL TRUE)

add_executable(error_extended_tests tests/error_extended_tests.cpp object.cpp)
target_include_directories(error_extended_tests PRIVATE ${PROJECT_SOURCE_DIR})
target_compile_options(error_extended_tests PRIVATE -Wall -Wextra -Wpedantic)
target_compile_definitions(error_extended_tests PRIVATE _GLIBCXX_ASSERTIONS)
add_test(NAME error_extended_contracts COMMAND error_extended_tests)
set_tests_properties(error_extended_contracts PROPERTIES
    LABELS error_regression TIMEOUT 10)
