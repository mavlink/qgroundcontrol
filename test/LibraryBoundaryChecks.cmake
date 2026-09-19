include_guard(GLOBAL)

# Add default-build checks for self-contained public FILE_SET headers.
function(qgc_check_library_headers target)
    if(NOT ARGC EQUAL 1)
        message(FATAL_ERROR "qgc_check_library_headers: expected exactly one target")
    endif()
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "qgc_check_library_headers: unknown target '${target}'")
    endif()

    set_target_properties(${target} PROPERTIES VERIFY_INTERFACE_HEADER_SETS ON)
    if(NOT TARGET "${target}HeaderChecks")
        add_custom_target(
            ${target}HeaderChecks ALL
            DEPENDS ${target}_verify_interface_header_sets
            COMMENT "Verify ${target} public headers in isolation"
        )
    endif()
endfunction()

# SOURCE, LABELS, and TIMEOUT are required; consumers inherit only the library's usage requirements.
function(qgc_add_library_consumer target)
    cmake_parse_arguments(PARSE_ARGV 1 ARG "" "SOURCE;TIMEOUT" "LABELS")
    if(DEFINED ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "qgc_add_library_consumer(${target}): missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(DEFINED ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_add_library_consumer(${target}): unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    foreach(keyword SOURCE LABELS TIMEOUT)
        if(NOT DEFINED ARG_${keyword} OR "${ARG_${keyword}}" STREQUAL "")
            message(FATAL_ERROR "qgc_add_library_consumer(${target}): ${keyword} is required")
        endif()
    endforeach()
    if(NOT ARG_TIMEOUT MATCHES "^[0-9]+([.][0-9]+)?$")
        message(FATAL_ERROR "qgc_add_library_consumer(${target}): TIMEOUT must be non-negative seconds")
    endif()

    qgc_check_library_headers("${target}")
    add_executable(${target}Consumer "${ARG_SOURCE}")
    set_target_properties(
        ${target}Consumer
        PROPERTIES AUTOMOC OFF
                   AUTOUIC OFF
                   AUTORCC OFF
    )
    target_link_libraries(${target}Consumer PRIVATE ${target})
    add_test(NAME ${target}Consumer COMMAND ${target}Consumer)
    set_tests_properties(${target}Consumer PROPERTIES LABELS "${ARG_LABELS}" TIMEOUT "${ARG_TIMEOUT}")
endfunction()
