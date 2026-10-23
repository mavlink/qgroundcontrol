cmake_minimum_required(VERSION 3.25)

# A fresh configure seeds the results an earlier configure saved for the same fingerprint, and
# saves only check results: never unrelated cache state or per-architecture type-size maps.

foreach(_required IN ITEMS QGC_MODULE_DIR FIXTURE_SOURCE_DIR FIXTURE_BINARY_DIR GENERATOR)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

set(_results_dir "${FIXTURE_BINARY_DIR}/results")
file(REMOVE_RECURSE "${FIXTURE_BINARY_DIR}")

# Configures the fixture into build-<name> and returns what it seeded and measured.
function(_qgc_configure_fixture name salt)
    set(_build "${FIXTURE_BINARY_DIR}/build-${name}")
    execute_process(
        COMMAND
            "${CMAKE_COMMAND}" --fresh -G "${GENERATOR}" -S "${FIXTURE_SOURCE_DIR}" -B "${_build}"
            "-DQGC_MODULE_DIR=${QGC_MODULE_DIR}" "-DQGC_CHECK_RESULT_CACHE_DIR=${_results_dir}"
            "-DQGC_CHECK_RESULT_CACHE_SALT=${salt}" ${ARGN}
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Check result cache fixture '${name}' failed to configure:\n${_output}${_error}")
    endif()
    file(READ "${_build}/seeded.txt" _seeded)
    file(READ "${_build}/results.txt" _results)
    set(seeded
        "${_seeded}"
        PARENT_SCOPE
    )
    set(results
        "${_results}"
        PARENT_SCOPE
    )
endfunction()

set(_all_checks FIXTURE_HAVE_STDIO_H FIXTURE_MISSING_HEADER FIXTURE_HAVE_PRINTF FIXTURE_SIZEOF_INT
                HAVE_FIXTURE_SIZEOF_INT FIXTURE_COMPILES
)

_qgc_configure_fixture(cold salt-a)
if(NOT seeded STREQUAL "")
    message(FATAL_ERROR "A cold configure seeded results: ${seeded}")
endif()
set(_cold_results "${results}")
if(NOT _cold_results MATCHES "^1\\|\\|1\\|[0-9]+\\|TRUE\\|1\\|#define FIXTURE_SIZEOF_INT [0-9]+$")
    message(FATAL_ERROR "Unexpected cold check results: ${_cold_results}")
endif()

file(GLOB _saved "${_results_dir}/*.cmake")
list(LENGTH _saved _saved_count)
if(NOT _saved_count EQUAL 1)
    message(FATAL_ERROR "Expected one saved result file, found: ${_saved}")
endif()
file(READ "${_saved}" _saved_content)
foreach(_var IN LISTS _all_checks)
    if(NOT _saved_content MATCHES "_qgc_check_result\\(\\[==\\[${_var}\\]==\\]")
        message(FATAL_ERROR "${_var} was not saved:\n${_saved_content}")
    endif()
endforeach()
foreach(_var IN ITEMS FIXTURE_UNRELATED FIXTURE_SIZE_MAP HAVE_FIXTURE_SIZE_MAP)
    if(_saved_content MATCHES "\\[==\\[${_var}\\]==\\]")
        message(FATAL_ERROR "${_var} must not be saved:\n${_saved_content}")
    endif()
endforeach()

_qgc_configure_fixture(warm salt-a)
if(NOT seeded STREQUAL _all_checks)
    message(FATAL_ERROR "A warm configure seeded '${seeded}', expected '${_all_checks}'")
endif()
if(NOT results STREQUAL _cold_results)
    message(FATAL_ERROR "Seeded results '${results}' differ from measured '${_cold_results}'")
endif()

_qgc_configure_fixture(explicit salt-a -DFIXTURE_HAVE_PRINTF:INTERNAL=0)
if(NOT results MATCHES "^1\\|\\|0\\|")
    message(FATAL_ERROR "A seed replaced an explicit -D value: ${results}")
endif()

_qgc_configure_fixture(other-salt salt-b)
if(NOT seeded STREQUAL "")
    message(FATAL_ERROR "Results seeded across fingerprints: ${seeded}")
endif()
file(GLOB _saved "${_results_dir}/*.cmake")
list(LENGTH _saved _saved_count)
if(NOT _saved_count EQUAL 2)
    message(FATAL_ERROR "Expected a second result file for the new fingerprint, found: ${_saved}")
endif()
