# Fails the check named <label> unless <actual> equals <expected>.
function(qgc_test_assert_streq label expected actual)
    if(NOT "${actual}" STREQUAL "${expected}")
        message(FATAL_ERROR "ASSERT [${label}]: expected='${expected}' actual='${actual}'")
    endif()
endfunction()

# Fails the check named <label> unless the list <list_var> contains <expected_item>.
function(qgc_test_assert_in_list label expected_item list_var)
    if(NOT "${expected_item}" IN_LIST ${list_var})
        message(FATAL_ERROR "ASSERT [${label}]: '${expected_item}' not in ${list_var}=${${list_var}}")
    endif()
endfunction()

# Fails the check named <label> if the list <list_var> contains <unexpected_item>.
function(qgc_test_assert_not_in_list label unexpected_item list_var)
    if("${unexpected_item}" IN_LIST ${list_var})
        message(FATAL_ERROR "ASSERT [${label}]: '${unexpected_item}' should not be in ${list_var}=${${list_var}}")
    endif()
endfunction()

# Fails the check named <label> unless target <tgt> exists.
function(qgc_test_assert_target label tgt)
    if(NOT TARGET ${tgt})
        message(FATAL_ERROR "ASSERT [${label}]: target '${tgt}' was not created")
    endif()
endfunction()

# Fails the check named <label> unless property <prop> of target <tgt> equals <expected>.
function(qgc_test_assert_property label tgt prop expected)
    get_target_property(_actual ${tgt} ${prop})
    if(NOT "${_actual}" STREQUAL "${expected}")
        message(FATAL_ERROR "ASSERT [${label}]: ${tgt}.${prop} expected='${expected}' actual='${_actual}'")
    endif()
endfunction()

# Fails the check named <label> unless <text>, with each run of whitespace collapsed to one space, matches <regex>.
function(qgc_test_assert_matches label regex text)
    string(REGEX REPLACE "[ \t\r\n]+" " " _normalized "${text}")
    if(NOT _normalized MATCHES "${regex}")
        message(FATAL_ERROR "ASSERT [${label}]: no match for '${regex}' in\n${text}")
    endif()
endfunction()

# Fails the check named <label> if <text>, with each run of whitespace collapsed to one space, matches <regex>.
function(qgc_test_assert_not_matches label regex text)
    string(REGEX REPLACE "[ \t\r\n]+" " " _normalized "${text}")
    if(_normalized MATCHES "${regex}")
        message(FATAL_ERROR "ASSERT [${label}]: unexpected match for '${regex}' in\n${text}")
    endif()
endfunction()

# Reports that the checks named <label> passed.
function(qgc_test_pass label)
    message(STATUS "PASS: ${label}")
endfunction()

# Sets <var> to an empty <name> directory in the working directory, removing what an earlier run left there. Tests leave
# it in place, so a failed run can be inspected.
function(qgc_test_sandbox var name)
    set(_dir "${CMAKE_CURRENT_BINARY_DIR}/${name}")
    file(REMOVE_RECURSE "${_dir}")
    file(MAKE_DIRECTORY "${_dir}")
    set(${var}
        "${_dir}"
        PARENT_SCOPE
    )
endfunction()

# Sets the variables <var>... in the caller to the "|"-separated fields of <case>, in order.
function(qgc_test_split case)
    string(REPLACE "|" ";" _fields "${case}")
    foreach(_var IN LISTS ARGN)
        list(POP_FRONT _fields _field)
        set(${_var}
            "${_field}"
            PARENT_SCOPE
        )
    endforeach()
endfunction()
