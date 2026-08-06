# JSON helpers for build-config.json access.

include_guard(GLOBAL)

# Read a JSON array at the given path into a CMake list. Empty if path absent.
# Usage: _qgc_json_array_to_list(OUT JSON_TEXT key1 key2 …)
function(_qgc_json_array_to_list OUTPUT_VAR JSON_TEXT)
    # Probe the root so malformed JSON FATALs here, distinct from an absent path
    # (which the LENGTH branch below correctly treats as an empty list).
    string(JSON _root_type ERROR_VARIABLE _root_err TYPE "${JSON_TEXT}")
    if(_root_err)
        message(FATAL_ERROR "_qgc_json_array_to_list: malformed JSON document: ${_root_err}")
    endif()
    string(JSON _count ERROR_VARIABLE _err LENGTH "${JSON_TEXT}" ${ARGN})
    set(_result "")
    if(NOT _err AND _count GREATER 0)
        math(EXPR _last "${_count} - 1")
        foreach(_i RANGE 0 ${_last})
            # GET returns raw sub-JSON for an OBJECT/ARRAY without erroring, so check
            # TYPE to keep nested structures out of the flat string list.
            string(JSON _type ERROR_VARIABLE _get_err TYPE "${JSON_TEXT}" ${ARGN} ${_i})
            if(_get_err)
                message(FATAL_ERROR "_qgc_json_array_to_list: failed to read element "
                    "at index ${_i} of path '${ARGN}': ${_get_err}")
            endif()
            if(_type STREQUAL "OBJECT" OR _type STREQUAL "ARRAY")
                message(FATAL_ERROR "_qgc_json_array_to_list: element at index ${_i} of "
                    "path '${ARGN}' is a non-scalar ${_type}; a scalar was expected.")
            endif()
            string(JSON _v GET "${JSON_TEXT}" ${ARGN} ${_i})
            # Empty list elements vanish in CMake (list(APPEND x "") is a no-op),
            # which would silently desync length/index semantics downstream.
            if(_v STREQUAL "")
                message(FATAL_ERROR "_qgc_json_array_to_list: empty array element at "
                    "index ${_i} of path '${ARGN}' is unsupported.")
            endif()
            list(APPEND _result "${_v}")
        endforeach()
    endif()
    set(${OUTPUT_VAR} "${_result}" PARENT_SCOPE)
endfunction()
