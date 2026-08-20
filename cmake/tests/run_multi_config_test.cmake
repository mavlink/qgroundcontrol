if(NOT QGC_MODULE_DIR
   OR NOT FIXTURE_SOURCE_DIR
   OR NOT FIXTURE_BINARY_DIR
)
    message(FATAL_ERROR "QGC_MODULE_DIR, FIXTURE_SOURCE_DIR, and FIXTURE_BINARY_DIR are required")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --fresh -G "Ninja Multi-Config" -S "${FIXTURE_SOURCE_DIR}" -B "${FIXTURE_BINARY_DIR}"
            "-DQGC_MODULE_DIR=${QGC_MODULE_DIR}"
    RESULT_VARIABLE _configure_result
    OUTPUT_VARIABLE _configure_output
    ERROR_VARIABLE _configure_error
)
if(NOT _configure_result EQUAL 0)
    message(FATAL_ERROR "Multi-config fixture configure failed:\n${_configure_output}${_configure_error}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${FIXTURE_BINARY_DIR}" --config Debug --target check-unit
    RESULT_VARIABLE _build_result
    OUTPUT_VARIABLE _build_output
    ERROR_VARIABLE _build_error
)
if(NOT _build_result EQUAL 0)
    message(FATAL_ERROR "Multi-config check target failed:\n${_build_output}${_build_error}")
endif()
if(NOT _build_output MATCHES "100% tests passed")
    message(FATAL_ERROR "Multi-config check target did not run its Debug test:\n${_build_output}")
endif()
