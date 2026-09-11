execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${FIXTURE}" -B "${OUTPUT_DIR}" "-DQGC_GPS_SOURCE_DIR=${QGC_GPS_SOURCE_DIR}"
            "-DCASE=${CASE}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(CASE STREQUAL "valid")
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Valid GPS dependency rejected: ${output}${error}")
    endif()
elseif(result EQUAL 0 OR NOT "${output}${error}" MATCHES "crosses the GPS library boundary")
    message(FATAL_ERROR "Invalid GPS dependency was not diagnosed: ${output}${error}")
endif()
