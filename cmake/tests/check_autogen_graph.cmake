cmake_minimum_required(VERSION 3.25)

# No project autogen step may wait for a linked library: that serialises the build behind the library's compile. The
# application's must still wait for the generated protocol headers its moc reads. Third-party steps (_deps/) keep
# CMake's default.
execute_process(
    COMMAND "${NINJA}" -C "${BUILD_DIR}" -t targets all
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _targets
    ERROR_VARIABLE _error
)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Cannot list build targets: ${_error}")
endif()
string(REGEX MATCHALL "[^\n:]+_autogen/timestamp:" _autogen_steps "${_targets}")
list(TRANSFORM _autogen_steps REPLACE ":$" "")
# Ninja also lists each step by absolute path.
list(FILTER _autogen_steps EXCLUDE REGEX "^(/|[A-Za-z]:|_deps/)")
set(_application_step "${APP_TARGET}_autogen/timestamp")
if(NOT _application_step IN_LIST _autogen_steps)
    message(FATAL_ERROR "Found no autogen step for ${APP_TARGET} in ${BUILD_DIR}")
endif()

execute_process(
    COMMAND "${NINJA}" -C "${BUILD_DIR}" -t query ${_autogen_steps}
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _graphs
    ERROR_VARIABLE _error
)
if(NOT _result EQUAL 0)
    message(FATAL_ERROR "Cannot inspect autogen dependencies: ${_error}")
endif()
# One section for each step: its name, then its indented inputs and outputs.
string(REGEX MATCHALL "[^\n]+:\n(  [^\n]*\n)+" _graphs "${_graphs}")
list(LENGTH _graphs _graph_count)
list(LENGTH _autogen_steps _autogen_step_count)
if(NOT _graph_count EQUAL _autogen_step_count)
    message(FATAL_ERROR "Expected ${_autogen_step_count} autogen dependency graphs, found ${_graph_count}")
endif()
foreach(_graph IN LISTS _graphs)
    if(_graph MATCHES "\\.(a|lib)(\n|\r)")
        message(FATAL_ERROR "Autogen waits for linked libraries:\n${_graph}")
    endif()
    string(FIND "${_graph}" "${_application_step}:" _application_graph)
    if(_application_graph EQUAL 0 AND NOT _graph MATCHES "qgc-analysis-headers")
        message(FATAL_ERROR "Application autogen is missing its generated-header prerequisite:\n${_graph}")
    endif()
endforeach()
message(STATUS "Checked ${_autogen_step_count} autogen steps")
