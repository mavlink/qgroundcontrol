# Readers for .github/build-config.json. They read QGC_BUILD_CONFIG_CONTENT, which cmake/BuildConfig.cmake loads from
# the file; script tests set it to a stand-in document.

include_guard(GLOBAL)

# qgc_config_read(<out_var> [LIST] [OPTIONAL] <key>...)
# Sets <out_var> to the scalar at the path of <key>s, one argument per object key (versions are keys and contain dots),
# or with LIST to the elements of the array there. A missing path fails configure, or yields "" under OPTIONAL.
function(qgc_config_read out_var)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "LIST;OPTIONAL" "" "")
    if(NOT ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_config_read: a key path is required")
    endif()
    if(NOT DEFINED QGC_BUILD_CONFIG_CONTENT)
        message(FATAL_ERROR "qgc_config_read: build-config.json is not loaded (include cmake/BuildConfig.cmake)")
    endif()
    set(_path ${ARG_UNPARSED_ARGUMENTS})
    list(JOIN _path "." _key)
    string(
        JSON
        _type
        ERROR_VARIABLE
        _error
        TYPE
        "${QGC_BUILD_CONFIG_CONTENT}"
        ${_path}
    )
    set(_value "")
    if(_error)
        if(NOT ARG_OPTIONAL)
            message(FATAL_ERROR "QGC: BuildConfig: Key '${_key}' not found in .github/build-config.json")
        endif()
    elseif(ARG_LIST)
        if(NOT _type STREQUAL "ARRAY")
            message(FATAL_ERROR "QGC: BuildConfig: Key '${_key}' must be an array, got ${_type}")
        endif()
        string(JSON _count LENGTH "${QGC_BUILD_CONFIG_CONTENT}" ${_path})
        if(_count GREATER 0)
            math(EXPR _last "${_count} - 1")
            foreach(_index RANGE 0 ${_last} 1)
                string(JSON _element GET "${QGC_BUILD_CONFIG_CONTENT}" ${_path} ${_index})
                list(APPEND _value "${_element}")
            endforeach()
        endif()
    elseif(_type STREQUAL "ARRAY" OR _type STREQUAL "OBJECT")
        message(FATAL_ERROR "QGC: BuildConfig: Key '${_key}' must be scalar, got ${_type}")
    else()
        string(JSON _value GET "${QGC_BUILD_CONFIG_CONTENT}" ${_path})
    endif()
    set(${out_var}
        "${_value}"
        PARENT_SCOPE
    )
endfunction()

# qgc_config_get_value(<var_name> <dotted.key>)
# Caches the scalar at <dotted.key>, a path whose keys contain no dots, as <var_name>.
function(qgc_config_get_value var_name json_key)
    if(NOT ARGC EQUAL 2
       OR NOT var_name MATCHES "^[A-Za-z_][A-Za-z0-9_]*$"
       OR NOT json_key
    )
        message(FATAL_ERROR "qgc_config_get_value: a valid output variable and non-empty JSON key are required")
    endif()
    string(REPLACE "." ";" _path "${json_key}")
    qgc_config_read(_value ${_path})
    set(${var_name}
        "${_value}"
        CACHE STRING "${json_key}" FORCE
    )
endfunction()
