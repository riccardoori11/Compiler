# Build only during this test so the known linker defect cannot break the
# normal build or prevent the other regression cases from running.
execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --config "${CONFIG}"
            --target error_type_link_probe
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors
    TIMEOUT 90)
if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "error 13: public type() caller failed to build/link:\n${output}\n${errors}")
endif()
execute_process(COMMAND "${PROBE}" RESULT_VARIABLE result
                OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 10)
if(NOT "${result}" STREQUAL "0" OR NOT "${output}" STREQUAL "int\n")
    message(FATAL_ERROR "error 13: type() caller failed: ${result}\n${output}\n${errors}")
endif()
