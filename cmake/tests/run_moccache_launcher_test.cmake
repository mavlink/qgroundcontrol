cmake_minimum_required(VERSION 3.25)

# Reconfiguring must not bump the moccache scripts' mtimes: AUTOMOC re-mocs every
# target when its executable (moccache-launcher) is newer than the generated files.

foreach(_required IN ITEMS QGC_MODULE_DIR FIXTURE_SOURCE_DIR FIXTURE_BINARY_DIR)
    if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
        message(FATAL_ERROR "${_required} is required")
    endif()
endforeach()

# Configures the fixture; extra arguments (e.g. --fresh) are passed to cmake.
function(_qgc_configure_fixture)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" ${ARGN} -S "${FIXTURE_SOURCE_DIR}" -B "${FIXTURE_BINARY_DIR}"
                "-DQGC_MODULE_DIR=${QGC_MODULE_DIR}"
        RESULT_VARIABLE _result
        OUTPUT_VARIABLE _output
        ERROR_VARIABLE _error
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "Moccache fixture configure failed:\n${_output}${_error}")
    endif()
endfunction()

_qgc_configure_fixture(--fresh)
file(READ "${FIXTURE_BINARY_DIR}/moccache-scripts.txt" _scripts)
if(_scripts STREQUAL ";")
    message(STATUS "QGC_TEST_SKIPPED: moccache prerequisites (python3 >= 3.10) not found")
    return()
endif()

set(_before "")
foreach(_script IN LISTS _scripts)
    file(TIMESTAMP "${_script}" _mtime "%Y-%m-%dT%H:%M:%S.%f" UTC)
    list(APPEND _before "${_mtime}")
endforeach()

_qgc_configure_fixture()

set(_index 0)
foreach(_script IN LISTS _scripts)
    list(GET _before ${_index} _old)
    file(TIMESTAMP "${_script}" _new "%Y-%m-%dT%H:%M:%S.%f" UTC)
    if(NOT _old STREQUAL _new)
        message(FATAL_ERROR "Reconfigure rewrote ${_script}: ${_old} -> ${_new}")
    endif()
    math(EXPR _index "${_index} + 1")
endforeach()
