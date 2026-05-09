# JSON helpers for build-config.json access.

# Read a JSON array at the given path into a CMake list. Empty if path absent.
# Usage: _qgc_json_array_to_list(OUT JSON_TEXT key1 key2 …)
function(_qgc_json_array_to_list OUTPUT_VAR JSON_TEXT)
    string(JSON _count ERROR_VARIABLE _err LENGTH "${JSON_TEXT}" ${ARGN})
    set(_result "")
    if(NOT _err AND _count GREATER 0)
        math(EXPR _last "${_count} - 1")
        foreach(_i RANGE 0 ${_last})
            string(JSON _v GET "${JSON_TEXT}" ${ARGN} ${_i})
            list(APPEND _result "${_v}")
        endforeach()
    endif()
    set(${OUTPUT_VAR} "${_result}" PARENT_SCOPE)
endfunction()
