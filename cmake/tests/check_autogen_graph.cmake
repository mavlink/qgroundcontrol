execute_process(
    COMMAND "${NINJA}" -C "${BUILD_DIR}" -t query "${APP_TARGET}_autogen/timestamp"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE graph
    ERROR_VARIABLE error
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Cannot inspect application autogen dependencies: ${error}")
endif()
if(graph MATCHES "\\.(a|lib)(\n|\r)")
    message(FATAL_ERROR "Application autogen waits for linked libraries:\n${graph}")
endif()
if(NOT graph MATCHES "qgc-analysis-headers")
    message(FATAL_ERROR "Application autogen is missing its generated-header prerequisite:\n${graph}")
endif()
