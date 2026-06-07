# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_disable_find_package_global_promotion target)
    set_target_properties("${target}" PROPERTIES _qt_no_promote_global TRUE)
endfunction()

# Transforms a list of package components into a string, so it can serve as a valid infix
# in a property name.
function(_qt_internal_get_package_components_as_valid_key_infix value out_var)
    string(REPLACE ";" "_" valid_value "${value}")
    set(${out_var} "${valid_value}" PARENT_SCOPE)
endfunction()

# This function computes a unique key / id using the package name and the components that are
# passed.
# The result is later used as property name, to store provided targets for a specific
# package + components combination.
function(_qt_internal_get_package_components_id)
    set(opt_args "")
    set(single_args
        PACKAGE_NAME
        OUT_VAR_KEY
    )
    set(multi_args
        COMPONENTS
        OPTIONAL_COMPONENTS
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_PACKAGE_NAME)
        message(FATAL_ERROR "PACKAGE_NAME is required")
    endif()

    if(NOT arg_OUT_VAR_KEY)
        message(FATAL_ERROR "OUT_VAR_KEY is required")
    endif()

    set(key "${arg_PACKAGE_NAME}")

    if(arg_COMPONENTS)
        _qt_internal_get_package_components_as_valid_key_infix("${arg_COMPONENTS}"
            components_as_string)
        string(APPEND key "-${components_as_string}")
    endif()

    if(arg_OPTIONAL_COMPONENTS)
        _qt_internal_get_package_components_as_valid_key_infix("${arg_OPTIONAL_COMPONENTS}"
            components_as_string)
        string(APPEND key "-${components_as_string}")
    endif()

    set(${arg_OUT_VAR_KEY} "${key}" PARENT_SCOPE)
endfunction()
