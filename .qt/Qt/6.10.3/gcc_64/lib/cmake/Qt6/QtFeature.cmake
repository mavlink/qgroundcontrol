# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(CheckCXXCompilerFlag)

function(qt_feature_module_begin)
    set(opt_args
        NO_MODULE
        NO_HEADERS
        ONLY_EVALUATE_FEATURES
    )
    set(single_args
        LIBRARY
        PRIVATE_FILE
        PUBLIC_FILE
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_ONLY_EVALUATE_FEATURES)
        if("${arg_LIBRARY}" STREQUAL "" AND (NOT ${arg_NO_MODULE}))
            message(FATAL_ERROR
                    "qt_feature_begin_module needs a LIBRARY name! (or specify NO_MODULE)")
        endif()
        if(NOT arg_NO_HEADERS)
            if("${arg_PUBLIC_FILE}" STREQUAL "")
                message(FATAL_ERROR
                    "qt_feature_begin_module needs a PUBLIC_FILE value "
                    "(or specify NO_HEADERS)")
            endif()
            if("${arg_PRIVATE_FILE}" STREQUAL "")
                message(FATAL_ERROR "qt_feature_begin_module needs a value "
                    "(or specify NO_HEADERS)")
            endif()
        endif()

        set(__QtFeature_only_evaluate_features OFF PARENT_SCOPE)
    else()
        set(__QtFeature_only_evaluate_features ON PARENT_SCOPE)
    endif()

    set(__QtFeature_library "${arg_LIBRARY}" PARENT_SCOPE)
    set(__QtFeature_public_features "" PARENT_SCOPE)
    set(__QtFeature_private_features "" PARENT_SCOPE)
    set(__QtFeature_internal_features "" PARENT_SCOPE)

    set(__QtFeature_private_file "${arg_PRIVATE_FILE}" PARENT_SCOPE)
    set(__QtFeature_public_file "${arg_PUBLIC_FILE}" PARENT_SCOPE)
    if(arg_NO_HEADERS)
        set(__QtFeature_no_headers "TRUE" PARENT_SCOPE)
    endif()

    set(__QtFeature_private_extra "" PARENT_SCOPE)
    set(__QtFeature_public_extra "" PARENT_SCOPE)

    set(__QtFeature_config_definitions "" PARENT_SCOPE)

    set(__QtFeature_define_definitions "" PARENT_SCOPE)
endfunction()

function(qt_feature feature)
    set(original_name "${feature}")
    qt_feature_normalize_name("${feature}" feature)
    set_property(GLOBAL PROPERTY QT_FEATURE_ORIGINAL_NAME_${feature} "${original_name}")

    set(no_value_options
        PRIVATE
        PUBLIC
        SYSTEM_LIBRARY
    )
    set(single_value_options
        LABEL
        PURPOSE
        SECTION
    )
    set(multi_value_options
        AUTODETECT
        CONDITION
        ENABLE
        DISABLE
        EMIT_IF
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg
        "${no_value_options}" "${single_value_options}" "${multi_value_options}"
    )
    _qt_internal_validate_all_args_are_parsed(arg)

    if(arg_SYSTEM_LIBRARY)
        # Enable SYSTEM_LIBRARY features if the 'force-system-libs' feature is enabled.
        if(DEFINED arg_ENABLE)
            list(PREPEND arg_ENABLE OR)
        endif()
        list(PREPEND arg_ENABLE QT_FEATURE_force_system_libs)

        # Disable SYSTEM_LIBRARY features if the 'force-bundled-libs' feature is enabled.
        if(DEFINED arg_DISABLE)
            list(PREPEND arg_DISABLE OR)
        endif()
        list(PREPEND arg_DISABLE QT_FEATURE_force_bundled_libs)

        qt_remove_args(forward_args
            ARGS_TO_REMOVE ENABLE DISABLE
            ALL_ARGS ${no_value_options} ${single_value_options} ${multi_value_options}
            ARGS ${ARGN}
        )

        list(APPEND forward_args
            ENABLE ${arg_ENABLE}
            DISABLE ${arg_DISABLE}
        )
    else()
        set(forward_args ${ARGN})
    endif()

    set(_QT_FEATURE_DEFINITION_${feature} ${forward_args} PARENT_SCOPE)

    # Register feature for future use:
    if (arg_PUBLIC)
        list(APPEND __QtFeature_public_features "${feature}")
    endif()
    if (arg_PRIVATE)
        list(APPEND __QtFeature_private_features "${feature}")
    endif()
    if (NOT arg_PUBLIC AND NOT arg_PRIVATE)
        list(APPEND __QtFeature_internal_features "${feature}")
    endif()


    set(__QtFeature_public_features ${__QtFeature_public_features} PARENT_SCOPE)
    set(__QtFeature_private_features ${__QtFeature_private_features} PARENT_SCOPE)
    set(__QtFeature_internal_features ${__QtFeature_internal_features} PARENT_SCOPE)
endfunction()

# Define an alternative alias for the main feature definition.
#
# If the main feature is defined, it takes precedence and all others are ignored. Otherwise
# only one alias may be defined.
#
# Currently, we need this to be a dedicated function because we are gathering all aliases of
# the main feature and check its real value as part of
# `qt_feature_check_and_save_user_provided_value`
#
# TODO: See how to move this into the main `qt_feature` definition flow.
# TODO: How to add `CONDITION` and how does it interact with the main feature's `EMIT_IF`
#
# Synopsis
#
#   qt_feature_alias(<alias_feature>
#       {ALIAS_OF_FEATURE <real_feature> | ALIAS_OF_CACHE <cache>}
#       [PRIVATE | PUBLIC]
#       [LABEL <string>]
#       [PURPOSE <string>]
#       [SECTION <string>]
#       [NEGATE]
#   )
#
# Arguments
#
# `<alias_feature>`
#   The feature alias to be created.
#
# `ALIAS_OF_FEATURE`
#   The true canonical feature to consider.
#
# `ALIAS_OF_CACHE`
#   The true cache variable that this value must synchronize with.
#
# `NEGATE`
#   Populate the main FEATURE variable with the opposite of the alias's value
#
# `LABEL`, `PURPOSE`, `SECTION`
#   Same as in `qt_feature`
#
# `PRIVATE`, `PUBLIC`
#   Same as in `qt_feature`. When `ALIAS_OF_FEATURE` is used, these have no effect, instead the
#   value of the the true feature is used.
function(qt_feature_alias alias_feature)
    set(option_args
        NEGATE
        PRIVATE
        PUBLIC
    )
    set(single_args
        ALIAS_OF_FEATURE
        ALIAS_OF_CACHE
        LABEL
        PURPOSE
        SECTION
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 1 arg "${option_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_ALIAS_OF_FEATURE AND NOT arg_ALIAS_OF_CACHE)
        message(FATAL_ERROR "One of ALIAS_OF_FEATURE,ALIAS_OF_CACHE must be passed.")
    endif()
    if(arg_ALIAS_OF_FEATURE AND arg_ALIAS_OF_CACHE)
        message(FATAL_ERROR "ALIAS_OF_FEATURE and ALIAS_OF_CACHE are mutually exclusive.")
    endif()

    set(original_name "${alias_feature}")
    qt_feature_normalize_name("${alias_feature}" alias_feature)
    set_property(GLOBAL PROPERTY QT_FEATURE_ORIGINAL_NAME_${alias_feature} "${original_name}")

    set(forward_args "")
    if(arg_ALIAS_OF_FEATURE)
        qt_feature_normalize_name("${arg_ALIAS_OF_FEATURE}" arg_ALIAS_OF_FEATURE)
        if(NOT DEFINED _QT_FEATURE_DEFINITION_${arg_ALIAS_OF_FEATURE})
            message(FATAL_ERROR "Primary feature ${arg_ALIAS_OF_FEATURE} was not defined yet.")
        endif()
        list(APPEND _QT_FEATURE_ALIASES_${arg_ALIAS_OF_FEATURE} "${alias_feature}")
        list(APPEND forward_args ALIAS_OF_FEATURE "${arg_ALIAS_OF_FEATURE}")
    endif()
    if(arg_ALIAS_OF_CACHE)
        list(APPEND forward_args ALIAS_OF_CACHE "${arg_ALIAS_OF_CACHE}")
    endif()
    if(arg_NEGATE)
        list(APPEND forward_args ALIAS_NEGATE)
    endif()
    if(arg_LABEL)
        list(APPEND forward_args LABEL "${arg_LABEL}")
    endif()
    if(arg_PURPOSE)
        list(APPEND forward_args PURPOSE "${arg_PURPOSE}")
    endif()
    if(arg_SECTION)
        list(APPEND forward_args SECTION "${arg_SECTION}")
    endif()

    set(_QT_FEATURE_DEFINITION_${alias_feature} ${forward_args} PARENT_SCOPE)

    if(arg_ALIAS_OF_FEATURE)
        # Register alias as a feature type similar to the true one
        foreach(type IN ITEMS public private internal)
            if(arg_ALIAS_OF_FEATURE IN_LIST __QtFeature_${type}_features)
                list(APPEND __QtFeature_${type}_features "${alias_feature}")
                set(__QtFeature_${type}_features ${__QtFeature_${type}_features} PARENT_SCOPE)

            endif()
        endforeach()
        set(_QT_FEATURE_ALIASES_${arg_ALIAS_OF_FEATURE}
            "${_QT_FEATURE_ALIASES_${arg_ALIAS_OF_FEATURE}}" PARENT_SCOPE)
    else()
        # Otherwise use the same logic as qt_feature
        if (arg_PUBLIC)
            list(APPEND __QtFeature_public_features "${alias_feature}")
        endif()
        if (arg_PRIVATE)
            list(APPEND __QtFeature_private_features "${alias_feature}")
        endif()
        if (NOT arg_PUBLIC AND NOT arg_PRIVATE)
            list(APPEND __QtFeature_internal_features "${alias_feature}")
        endif()

        set(__QtFeature_public_features ${__QtFeature_public_features} PARENT_SCOPE)
        set(__QtFeature_private_features ${__QtFeature_private_features} PARENT_SCOPE)
        set(__QtFeature_internal_features ${__QtFeature_internal_features} PARENT_SCOPE)
    endif()
endfunction()

# Create a deprecated feature
#
# Synopsis
#
#   qt_feature_deprecated(<feature>
#       REMOVE_BY <version>
#       [MESSAGE <string>] [VALUE <val>]
#       [PRIVATE | PUBLIC]
#       [LABEL <string>] [PURPOSE <string>] [SECTION <string>]
#   )
#
# Arguments
#
# `<feature>`
#   The feature to be created.
#
# `REMOVE_BY`
#   Qt version when this feature is going to be removed
#
# `MESSAGE`
#   Additional deprecation message to be printed.
#
# `VALUE`
#   Value of the `QT_FEATURE_<feature>` that this is forced to. If undefined,
#   `QT_FEATURE_<feature>` is not populated
#
# `LABEL`, `PURPOSE`, `SECTION`, `PRIVATE`, `PUBLIC`
#   Same as in `qt_feature`
function(qt_feature_deprecated feature)
    set(option_args
        PRIVATE
        PUBLIC
    )
    set(single_args
        REMOVE_BY
        MESSAGE
        VALUE
        LABEL
        PURPOSE
        SECTION
    )
    set(multi_args "")

    cmake_parse_arguments(PARSE_ARGV 1 arg "${option_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_REMOVE_BY)
        message(FATAL_ERROR "qt_feature_deprecated requires REMOVE_BY keyword")
    elseif(PROJECT_VERSION VERSION_GREATER_EQUAL arg_REMOVE_BY)
        message(FATAL_ERROR
            "Deprecated feature ${feature} must be removed before Qt version ${arg_REMOVE_BY}"
        )
    endif()

    set(original_name "${feature}")
    qt_feature_normalize_name("${feature}" feature)

    # Check if the values were manually passed
    if(DEFINED FEATURE_${feature})
        set(deprecation_msg "FEATURE_${feature} is deprecated. ")
        if(arg_VALUE)
            string(APPEND deprecation_msg "The value is always: ${arg_VALUE}")
        else()
            string(APPEND deprecation_msg "The value is not used.")
        endif()
        if(arg_MESSAGE)
            string(APPEND deprecation_msg "\n${arg_MESSAGE}")
        endif()
        qt_configure_add_report_entry(RECORD_ON_FEATURE_EVALUATION TYPE WARNING
            MESSAGE "${deprecation_msg}")
        unset(FEATURE_${feature} CACHE)
    endif()

    # Make sure the `QT_FEATURE_*` value is set/unset accordingly
    unset(err_msg)
    if(arg_VALUE)
        if(DEFINED QT_FEATURE_${feature} AND NOT QT_FEATURE_${feature} STREQUAL arg_VALUE)
            string(CONCAT err_msg
                "QT_FEATURE_${feature} was manually set to ${QT_FEATURE_${feature}}, but"
                "the only supported value is: ${arg_VALUE}\n"
                "Overwriting QT_FEATURE_${feature} cache to ${arg_VALUE}"
            )
        endif()
        set(QT_FEATURE_${feature} "${arg_VALUE}" CACHE INTERNAL
            "Deprecated: Always ${arg_VALUE}. ${arg_MESSAGE}"
        )
    else()
        if(DEFINED QT_FEATURE_${feature})
            string(CONCAT msg
                "QT_FEATURE_${feature} was manually set to ${QT_FEATURE_${feature}}, but"
                "the value must **NOT** be set.\n"
                "Unsetting QT_FEATURE_${feature} cache"
            )
            unset(QT_FEATURE_${feature} CACHE)
        endif()
    endif()

    # Emit the error message if we have an unexpected `QT_FEATURE_*`
    if(err_msg)
        if(arg_MESSAGE)
            string(APPEND err_msg "\n${arg_MESSAGE}")
        endif()
        qt_configure_add_report_error("${err_msg}")
    endif()

    # Register the feature as a normal feature
    set(forward_args "")
    foreach(arg IN ITEMS LABEL PURPOSE SECTION)
        if(arg_${arg})
            list(APPEND forward_args ${arg} "${arg_${arg}}")
        endif()
    endforeach()
    set(_QT_FEATURE_DEFINITION_${feature} ${forward_args} PARENT_SCOPE)

    # Do the feature register
    if (arg_PUBLIC)
        list(APPEND __QtFeature_public_features "${feature}")
        set(__QtFeature_public_features ${__QtFeature_public_features} PARENT_SCOPE)
    endif()
    if (arg_PRIVATE)
        list(APPEND __QtFeature_private_features "${feature}")
        set(__QtFeature_private_features ${__QtFeature_private_features} PARENT_SCOPE)
    endif()
    if (NOT arg_PUBLIC AND NOT arg_PRIVATE)
        list(APPEND __QtFeature_internal_features "${feature}")
        set(__QtFeature_internal_features ${__QtFeature_internal_features} PARENT_SCOPE)
    endif()
endfunction()

function(qt_evaluate_to_boolean expressionVar)
    if(${${expressionVar}})
        set(${expressionVar} ON PARENT_SCOPE)
    else()
        set(${expressionVar} OFF PARENT_SCOPE)
    endif()
endfunction()

function(qt_internal_evaluate_config_expression resultVar outIdx startIdx)
    set(result "")
    set(expression "${ARGN}")
    list(LENGTH expression length)
    get_property(known_compile_tests GLOBAL PROPERTY _qtfeature_known_compile_tests)

    math(EXPR memberIdx "${startIdx} - 1")
    math(EXPR length "${length}-1")
    while(memberIdx LESS ${length})
        math(EXPR memberIdx "${memberIdx} + 1")
        list(GET expression ${memberIdx} member)

        if("${member}" STREQUAL "(")
            math(EXPR memberIdx "${memberIdx} + 1")
            qt_internal_evaluate_config_expression(sub_result memberIdx ${memberIdx} ${expression})
            list(APPEND result ${sub_result})
        elseif("${member}" STREQUAL ")")
            break()
        elseif("${member}" STREQUAL "NOT")
            list(APPEND result ${member})
        elseif("${member}" STREQUAL "AND")
            qt_evaluate_to_boolean(result)
            if(NOT ${result})
                break()
            endif()
            set(result "")
        elseif("${member}" STREQUAL "OR")
            qt_evaluate_to_boolean(result)
            if(${result})
                break()
            endif()
            set(result "")
        elseif("${member}" STREQUAL "STREQUAL" AND memberIdx LESS ${length})
            # Unfortunately the semantics for STREQUAL in if() are broken when the
            # RHS is an empty string and the parameters to if are coming through a variable.
            # So we expect people to write the empty string with single quotes and then we
            # do the comparison manually here.
            list(LENGTH result lhsIndex)
            math(EXPR lhsIndex "${lhsIndex}-1")
            list(GET result ${lhsIndex} lhs)
            list(REMOVE_AT result ${lhsIndex})
            set(lhs "${${lhs}}")

            math(EXPR rhsIndex "${memberIdx}+1")
            set(memberIdx ${rhsIndex})

            list(GET expression ${rhsIndex} rhs)
            # We can't pass through an empty string with double quotes through various
            # stages of substitution, so instead it is represented using single quotes
            # and resolve here.
            string(REGEX REPLACE "'(.*)'" "\\1" rhs "${rhs}")

            string(COMPARE EQUAL "${lhs}" "${rhs}" stringCompareResult)
            list(APPEND result ${stringCompareResult})
        else()
            if(member MATCHES "^TEST_")
                # Remove the TEST_ prefix
                string(SUBSTRING "${member}" 5 -1 test_name)
                if(test_name IN_LIST known_compile_tests)
                    qt_run_config_compile_test("${test_name}")
                endif()
            elseif(member MATCHES "^QT_FEATURE_")
                # Remove the QT_FEATURE_ prefix
                string(SUBSTRING "${member}" 11 -1 feature)
                qt_evaluate_feature(${feature})
            endif()

            list(APPEND result ${member})
        endif()
    endwhile()
    # The 'TARGET Gui' case is handled by qt_evaluate_to_boolean, by passing those tokens verbatim
    # to if().

    if("${result}" STREQUAL "")
        set(result ON)
    else()
        qt_evaluate_to_boolean(result)
    endif()

    # When in recursion, we must skip to the next closing parenthesis on nesting level 0. The outIdx
    # must point to the matching closing parenthesis, and that's not the case if we're early exiting
    # in AND/OR.
    if(startIdx GREATER 0)
        set(nestingLevel 1)
        while(TRUE)
            list(GET expression ${memberIdx} member)
            if("${member}" STREQUAL ")")
                math(EXPR nestingLevel "${nestingLevel} - 1")
                if(nestingLevel EQUAL 0)
                    break()
                endif()
            elseif("${member}" STREQUAL "(")
                math(EXPR nestingLevel "${nestingLevel} + 1")
            endif()
            math(EXPR memberIdx "${memberIdx} + 1")
        endwhile()
    endif()

    set(${outIdx} ${memberIdx} PARENT_SCOPE)
    set(${resultVar} ${result} PARENT_SCOPE)
endfunction()

function(qt_evaluate_config_expression resultVar)
    qt_internal_evaluate_config_expression(result unused 0 ${ARGN})
    set("${resultVar}" "${result}" PARENT_SCOPE)
endfunction()

function(_qt_internal_get_feature_condition_keywords out_var)
    set(keywords "EQUAL" "LESS" "LESS_EQUAL" "GREATER" "GREATER_EQUAL" "STREQUAL" "STRLESS"
        "STRLESS_EQUAL" "STRGREATER" "STRGREATER_EQUAL" "VERSION_EQUAL" "VERSION_LESS"
        "VERSION_LESS_EQUAL" "VERSION_GREATER" "VERSION_GREATER_EQUAL" "MATCHES"
        "EXISTS" "COMMAND" "DEFINED" "NOT" "AND" "OR" "TARGET" "EXISTS" "IN_LIST" "(" ")")
    set(${out_var} "${keywords}" PARENT_SCOPE)
endfunction()

function(_qt_internal_dump_expression_values expression_dump expression)
    set(dump "")
    set(skipNext FALSE)
    set(isTargetExpression FALSE)

    _qt_internal_get_feature_condition_keywords(keywords)

    list(LENGTH expression length)
    math(EXPR length "${length}-1")

    if(${length} LESS 0)
        return()
    endif()

    foreach(memberIdx RANGE ${length})
        if(${skipNext})
            set(skipNext FALSE)
            continue()
        endif()

        list(GET expression ${memberIdx} member)
        if(NOT "${member}" IN_LIST keywords)
            string(FIND "${member}" "QT_FEATURE_" idx)
            if(idx EQUAL 0)
                if(NOT DEFINED ${member})
                    list(APPEND dump "${member} not evaluated")
                else()
                    list(APPEND dump "${member} = \"${${member}}\"")
                endif()
            elseif(isTargetExpression)
                set(targetExpression "TARGET;${member}")
                if(${targetExpression})
                    list(APPEND dump "TARGET ${member} found")
                else()
                    list(APPEND dump "TARGET ${member} not found")
                endif()
            else()
                list(APPEND dump "${member} = \"${${member}}\"")
            endif()
            set(isTargetExpression FALSE)
            set(skipNext FALSE)
        elseif("${member}" STREQUAL "TARGET")
            set(isTargetExpression TRUE)
        elseif("${member}" STREQUAL "STREQUAL")
            set(skipNext TRUE)
        else()
            set(skipNext FALSE)
            set(isTargetExpression FALSE)
        endif()
    endforeach()
    string(JOIN "\n    " ${expression_dump} ${dump})
    set(${expression_dump} "${${expression_dump}}" PARENT_SCOPE)
endfunction()

# Check the actual value of a given feature/alias.
# The value can come from `FEATURE_<alias>` being set or from another alias.
# The out_var is sanitized to 0/1 and it is not set if the feature was not defined.
function(_qt_feature_evaluate_alias out_var alias)
    # Evaluate the alias against the aliases of the alias
    _qt_feature_check_feature_alias(${alias})
    if(NOT DEFINED FEATURE_${alias})
        # If the alias was not defined, don't set value
        return()
    endif()
    # Check if we need to negate the value to be set or not
    set(not_kw)
    _qt_internal_parse_feature_definition("${alias}")
    if(arg_ALIAS_NEGATE)
        set(not_kw "NOT")
    endif()
    # Evaluate the value and return it
    if(${not_kw} FEATURE_${alias})
        set(${out_var} 1)
    else()
        set(${out_var} 0)
    endif()
    set(${out_var} "${${out_var}}" PARENT_SCOPE)
    # Also set `not_kw` since it would be reused by the caller
    set(not_kw "${not_kw}" PARENT_SCOPE)
endfunction()

# Check that the feature value is consistent with any of its aliases.
# If the feature was not set via `FEATURE_<feature>`, it may be set by the aliases.
function(_qt_feature_check_feature_alias feature)
    if(DEFINED "FEATURE_${feature}")
        # The main feature was already defined, use the current value.
        # Just check if the other aliases have agreeing values.
        if(FEATURE_${feature})
            set(expected_value 1)
        else()
            set(expected_value 0)
        endif()
        unset(alias_value)
        foreach(alias IN LISTS _QT_FEATURE_ALIASES_${feature})
            _qt_feature_evaluate_alias(alias_value ${alias})
            if(DEFINED alias_value)
                if(NOT expected_value EQUAL alias_value)
                    string(CONCAT msg
                        "Alias FEATURE_${alias}(${FEATURE_${alias}}) is an alias of ${not_kw} "
                        "FEATURE_${feature}(${FEATURE_${alias}}), and their values conflict"
                    )
                    qt_configure_add_report_error(${msg})
                endif()
                unset(alias_value)
            endif()
        endforeach()
        return()
    endif()
    # Otherwise try to set the `feature` value from the aliases
    set(aliases_set "")
    set(expected_value "")
    set(alias_not_kws "")
    unset(value)
    foreach(alias IN LISTS _QT_FEATURE_ALIASES_${feature})
        _qt_feature_evaluate_alias(value ${alias})
        if(DEFINED value)
            list(APPEND aliases_set "${alias}")
            list(APPEND expected_value "${value}")
            list(APPEND alias_not_kws "${not_kw}")
            unset(value)
        endif()
    endforeach()
    # If there were no aliases set, do not set the feature either
    if(NOT aliases_set)
        return()
    endif()
    # Check if all values are consistent
    list(REMOVE_DUPLICATES expected_value)
    list(LENGTH expected_value expected_value_length)
    if(expected_value_length GREATER 1)
        string(CONCAT msg
            "Multiple aliases of FEATURE_${feature} were defined, with conflicting values.\n"
            "Aliases:"
        )
        while(aliases_set)
            list(POP_FRONT aliases_set alias)
            list(POP_FRONT alias_not_kws not_kw)
            string(CONCAT msg
                "${msg}\n"
                "  - ${not_kw} FEATURE_${alias}(${FEATURE_${alias}})"
            )
        endwhile()
        qt_configure_add_report_error(${msg})
        return()
    endif()
    # All aliased values are agreeing
    set("FEATURE_${feature}" ${expected_value} PARENT_SCOPE)
endfunction()

# Check that the alias value is consistent with its cache source
function(_qt_feature_check_cache_alias feature)
    _qt_internal_parse_feature_definition("${feature}")
    if(NOT arg_ALIAS_OF_CACHE)
        # Not an alias of a cache variable, skip this check
        return()
    endif()
    if(DEFINED "${arg_ALIAS_OF_CACHE}")
        # Check if the feature is set by another alias
        unset(expected_value)
        _qt_feature_evaluate_alias(expected_value "${feature}")
        if(${arg_ALIAS_OF_CACHE})
            set(cache_sanitized 1)
        else()
            set(cache_sanitized 0)
        endif()
        if(NOT DEFINED expected_value)
            # If nothing else set the alias value, use the primary cache value
            set("FEATURE_${feature}" "${${arg_ALIAS_OF_CACHE}}" PARENT_SCOPE)
            return()
        endif()
        # Otherwise, just check for consistency
        if(NOT cache_sanitized EQUAL expected_value)
            string(CONCAT msg
                "FEATURE_${feature}(${FEATURE_${feature}}) is an alias of ${not_kw} "
                "${arg_ALIAS_OF_CACHE}(${${arg_ALIAS_OF_CACHE}}), and their values conflict."
            )
            qt_configure_add_report_error(${msg})
        endif()
    endif()
endfunction()

# Make sure all the direct alias values are set after the final evaluation of the feature value.
# Must be run after `_qt_feature_check_feature_alias` which checks for user-defined values and
# any inconsistencies
function(_qt_feature_save_alias feature)
    foreach(alias IN LISTS _QT_FEATURE_ALIASES_${feature})
        # We only need to set the alias values if they were not explicitly set. The consistency
        # check for those was done prior to this function call.
        if(NOT DEFINED FEATURE_${alias})
            # Evaluate the alias value based on the original
            set(not_kw)
            _qt_internal_parse_feature_definition("${alias}")
            if(arg_ALIAS_NEGATE)
                set(not_kw "NOT")
            endif()
            if(${not_kw} FEATURE_${feature})
                set(value 1)
            else()
                set(value 0)
            endif()
            qt_evaluate_to_boolean(value)
            # Set the values based on the main feature's value
            set(FEATURE_${alias} ${value} CACHE BOOL
                # Using a temporary docstring that should be overwritten if everything works well
                "(temporarily set by _qt_feature_save_alias)"
            )
        endif()
    endforeach()
endfunction()

# Stores the user provided value to FEATURE_${feature} if provided.
# If not provided, stores ${computed} instead.
# ${computed} is also stored when reconfiguring and the condition does not align with the user
# provided value.
#
function(qt_feature_check_and_save_user_provided_value
        resultVar feature condition condition_expression computed label)
    _qt_feature_check_cache_alias(${feature})
    _qt_feature_check_feature_alias(${feature})
    if (DEFINED "FEATURE_${feature}")
        # Revisit new user provided value
        set(user_value "${FEATURE_${feature}}")
        string(TOUPPER "${user_value}" user_value_upper)
        set(result "${user_value_upper}")

        # If ${feature} depends on another dirty feature, reset the ${feature} value to
        # ${computed}.
        get_property(dirty_build GLOBAL PROPERTY _qt_dirty_build)
        if(dirty_build)
            _qt_internal_feature_compute_feature_dependencies(deps "${feature}")
            if(deps)
                get_property(dirty_features GLOBAL PROPERTY _qt_dirty_features)
                foreach(dirty_feature ${dirty_features})
                    if(dirty_feature IN_LIST deps AND NOT "${result}" STREQUAL "${computed}")
                        set(result "${computed}")
                        message(WARNING
                            "Auto-resetting 'FEATURE_${feature}' from '${user_value_upper}' to "
                            "'${computed}', "
                            "because the dependent feature '${dirty_feature}' was marked dirty.")

                        # Append ${feature} as a new dirty feature.
                        set_property(GLOBAL APPEND PROPERTY _qt_dirty_features "${feature}")
                        break()
                    endif()
                endforeach()
            endif()

            # If the build is marked as dirty and the feature doesn't meet its condition,
            # reset its value to the computed one, which is likely OFF.
            if(NOT condition AND result)
                set(result "${computed}")
                message(WARNING "Resetting 'FEATURE_${feature}' from '${user_value_upper}' to "
                    "'${computed}' because it doesn't meet its condition after reconfiguration. "
                    "Condition expression is: '${condition_expression}'")
            endif()
        endif()

        set(bool_values OFF NO FALSE N ON YES TRUE Y)
        if ((result IN_LIST bool_values) OR (result GREATER_EQUAL 0))
            # All good!
        else()
            message(FATAL_ERROR
                "Sanity check failed: FEATURE_${feature} has invalid value \"${result}\"!")
        endif()

        # Fix-up user-provided values
        set("FEATURE_${feature}" "${result}" CACHE BOOL "${label}" FORCE)
    else()
        # Initial setup:
        set(result "${computed}")
        set("FEATURE_${feature}" "${result}" CACHE BOOL "${label}")
        # Update the HELPSTRING if needed
        set_property(CACHE "FEATURE_${feature}" PROPERTY
            HELPSTRING "${label}"
        )
    endif()

    # Check for potential typo
    get_property(original_name GLOBAL PROPERTY "QT_FEATURE_ORIGINAL_NAME_${feature}")
    if(NOT original_name STREQUAL feature AND DEFINED "FEATURE_${original_name}")
        unset("FEATURE_${original_name}" CACHE)
        qt_configure_add_report_error(
            "FEATURE_${original_name} does not exist. Consider using: FEATURE_${feature}"
        )
    endif()

    # Set the values of each direct alias
    _qt_feature_save_alias("${feature}")

    set("${resultVar}" "${result}" PARENT_SCOPE)
endfunction()

# Saves the final user value to QT_FEATURE_${feature}, after checking that the condition is met.
macro(qt_feature_check_and_save_internal_value
        feature saved_user_value condition label conditionExpression)
    if(${saved_user_value})
        set(result ON)
    else()
        set(result OFF)
    endif()

    if ((NOT condition) AND result)
        _qt_internal_dump_expression_values(conditionDump "${conditionExpression}")
        string(JOIN " " conditionString ${conditionExpression})
        qt_configure_add_report_error("Feature \"${feature}\": Forcing to \"${result}\" breaks its \
condition:\n    ${conditionString}\nCondition values dump:\n    ${conditionDump}\n" RECORD_ON_FEATURE_EVALUATION)
    endif()

    if (DEFINED "QT_FEATURE_${feature}")
        message(FATAL_ERROR "Feature ${feature} is already defined when evaluating configure.cmake features for ${target}.")
    endif()
    set(QT_FEATURE_${feature} "${result}" CACHE INTERNAL "Qt feature: ${feature}")

    # Add feature to build feature collection
    list(APPEND QT_KNOWN_FEATURES "${feature}")
    set(QT_KNOWN_FEATURES "${QT_KNOWN_FEATURES}" CACHE INTERNAL "" FORCE)
endmacro()

macro(_qt_internal_parse_feature_definition feature)
    cmake_parse_arguments(arg
        "PRIVATE;PUBLIC;ALIAS_NEGATE"
        "LABEL;PURPOSE;SECTION;ALIAS_OF_FEATURE;ALIAS_OF_CACHE"
        "AUTODETECT;CONDITION;ENABLE;DISABLE;EMIT_IF"
        ${_QT_FEATURE_DEFINITION_${feature}})
endmacro()


# The build system stores 2 CMake cache variables for each feature, to allow detecting value changes
# during subsequent reconfigurations.
#
#
# `FEATURE_foo` stores the user provided feature value for the current configuration run.
# It can be set directly by the user.
#
# If a value is not provided on initial configuration, the value will be auto-computed based on the
# various conditions of the feature.
# TODO: Document the various conditions and how they relate to each other.
#
#
# `QT_FEATURE_foo` stores the value of the feature from the previous configuration run.
# Its value is updated once with the newest user provided value after some checks are performed.
#
# This variable also serves as the main source of truth throughout the build system code to check
# if the feature is enabled, e.g. if(QT_FEATURE_foo)
#
# It is not meant to be set by the user. It is only modified by the build system.
#
# Comparing the values of QT_FEATURE_foo and FEATURE_foo, the build system can detect whether
# the user changed the value for a feature and thus recompute any dependent features.
#
function(qt_evaluate_feature feature)
    # If the feature was already evaluated as dependency nothing to do here.
    if(DEFINED "QT_FEATURE_${feature}")
        return()
    endif()

    if(NOT DEFINED _QT_FEATURE_DEFINITION_${feature})
        qt_debug_print_variables(DEDUP MATCH "^QT_FEATURE")
        message(FATAL_ERROR "Attempting to evaluate feature ${feature} but its definition is missing. Either the feature does not exist or a dependency to the module that defines it is missing")
    endif()

    _qt_internal_parse_feature_definition("${feature}")

    if(arg_ALIAS_OF)
        # If the current feature is an alias of another, we have to check the original source first
        # in order to be consistent with the original source if set
        qt_evaluate_feature("${arg_ALIAS_OF}")
    endif()

    if("${arg_ENABLE}" STREQUAL "")
        set(arg_ENABLE OFF)
    endif()

    if("${arg_DISABLE}" STREQUAL "")
        set(arg_DISABLE OFF)
    endif()

    if("${arg_AUTODETECT}" STREQUAL "")
        set(arg_AUTODETECT ON)
    endif()

    if("${arg_CONDITION}" STREQUAL "")
        set(condition ON)
    else()
        qt_evaluate_config_expression(condition ${arg_CONDITION})
    endif()

    qt_evaluate_config_expression(disable_result ${arg_DISABLE})
    qt_evaluate_config_expression(enable_result ${arg_ENABLE})
    qt_evaluate_config_expression(auto_detect ${arg_AUTODETECT})
    if(${disable_result})
        set(computed OFF)
    elseif(${enable_result})
        set(computed ON)
    elseif(${auto_detect})
        set(computed ${condition})
    else()
        # feature not auto-detected and not explicitly enabled
        set(computed OFF)
    endif()

    if("${arg_EMIT_IF}" STREQUAL "")
        set(emit_if ON)
    else()
        qt_evaluate_config_expression(emit_if ${arg_EMIT_IF})
    endif()

    set(actual_label "${arg_LABEL}")
    if(arg_ALIAS_OF_FEATURE OR arg_ALIAS_OF_CACHE)
        set(not_kw)
        if(arg_ALIAS_NEGATE)
            set(not_kw "NOT")
        endif()
        # Only one of ALIAS_OF_FEATURE/ALIAS_OF_CACHE will be set, so we can just combine them
        set(alias_source "${not_kw} ${arg_ALIAS_OF_FEATURE}${arg_ALIAS_OF_CACHE}")
        string(APPEND actual_label " (alias of ${alias_source})")
    endif()

    # Warn about a feature which is not emitted, but the user explicitly provided a value for it.
    if(NOT emit_if AND DEFINED FEATURE_${feature})
        set(msg "")
        string(APPEND msg
            "Feature ${feature} is insignificant in this configuration, "
            "ignoring related command line option(s).")
        qt_configure_add_report_entry(TYPE WARNING MESSAGE "${msg}")

        # Remove the cache entry so that the warning is not persisted and shown on every
        # reconfiguration.
        unset(FEATURE_${feature} CACHE)
    endif()

    # Only save the user provided value if the feature was emitted.
    if(emit_if)
        qt_feature_check_and_save_user_provided_value(
            saved_user_value
            "${feature}" "${condition}" "${arg_CONDITION}" "${computed}" "${actual_label}")
    else()
        # Make sure the feature internal value is OFF if not emitted.
        set(saved_user_value OFF)
    endif()

    qt_feature_check_and_save_internal_value(
        "${feature}" "${saved_user_value}" "${condition}" "${actual_label}" "${arg_CONDITION}")

    # Store each feature's label for summary info.
    set(QT_FEATURE_LABEL_${feature} "${actual_label}" CACHE INTERNAL "")
endfunction()

# Collect feature names that ${feature} depends on, by inspecting the given expression.
function(_qt_internal_feature_extract_feature_dependencies_from_expression out_var expression)
    list(LENGTH expression length)
    math(EXPR length "${length}-1")

    if(length LESS 0)
        set(${out_var} "" PARENT_SCOPE)
        return()
    endif()

    set(deps "")

    foreach(memberIdx RANGE ${length})
        list(GET expression ${memberIdx} member)
        if(member MATCHES "^QT_FEATURE_(.+)")
            list(APPEND deps "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    set(${out_var} "${deps}" PARENT_SCOPE)
endfunction()

# Collect feature names that ${feature} depends on, based on feature names that appear
# in the ${feature}'s condition expressions.
function(_qt_internal_feature_compute_feature_dependencies out_var feature)
    # Only compute the deps once per feature.
    get_property(deps_computed GLOBAL PROPERTY _qt_feature_deps_computed_${feature})
    if(deps_computed)
        get_property(deps GLOBAL PROPERTY _qt_feature_deps_${feature})
        set(${out_var} "${deps}" PARENT_SCOPE)
        return()
    endif()

    _qt_internal_parse_feature_definition("${feature}")

    set(options_to_check AUTODETECT CONDITION ENABLE DISABLE EMIT_IF)
    set(deps "")

    # Go through each option that takes condition expressions and collect the feature names.
    foreach(option ${options_to_check})
        set(option_value "${arg_${option}}")
        if(option_value)
            _qt_internal_feature_extract_feature_dependencies_from_expression(
                option_deps "${option_value}")
            if(option_deps)
                list(APPEND deps ${option_deps})
            endif()
        endif()
    endforeach()

    set_property(GLOBAL PROPERTY _qt_feature_deps_computed_${feature} TRUE)
    set_property(GLOBAL PROPERTY _qt_feature_deps_${feature} "${deps}")
    set(${out_var} "${deps}" PARENT_SCOPE)
endfunction()

function(qt_feature_config feature config_var_name)
    qt_feature_normalize_name("${feature}" feature)
    cmake_parse_arguments(PARSE_ARGV 2 arg
        "NEGATE"
        "NAME"
        "")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Store all the config related info in a unique variable key.
    set(key_name "_QT_FEATURE_CONFIG_DEFINITION_${feature}_${config_var_name}")
    set(${key_name} "FEATURE;${feature};CONFIG_VAR_NAME;${config_var_name};${ARGN}" PARENT_SCOPE)

    # Store the key for later evaluation.
    list(APPEND __QtFeature_config_definitions "${key_name}")

    set(__QtFeature_config_definitions ${__QtFeature_config_definitions} PARENT_SCOPE)
endfunction()

function(qt_evaluate_qmake_config_values key)
    if(NOT DEFINED ${key})
        qt_debug_print_variables(DEDUP MATCH "^_QT_FEATURE_CONFIG_DEFINITION")
        message(FATAL_ERROR
                "Attempting to evaluate feature config ${key} but its definition is missing. ")
    endif()

    cmake_parse_arguments(arg
        "NEGATE"
        "FEATURE;NAME;CONFIG_VAR_NAME"
        "" ${${key}})

    # If no custom name is specified, then the config value is the same as the feature name.
    if(NOT arg_NAME)
        set(arg_NAME "${arg_FEATURE}")
    endif()

    set(expected "NOT")
    if (arg_NEGATE)
        set(expected "")
        if(arg_NAME MATCHES "^no_(.*)")
            set(arg_NAME "${CMAKE_MATCH_1}")
        else()
            string(PREPEND arg_NAME "no_")
        endif()
    endif()

    # The feature condition is false, there is no need to export any config values.
    if(${expected} ${QT_FEATURE_${arg_FEATURE}})
        return()
    endif()

    if(arg_CONFIG_VAR_NAME STREQUAL "QMAKE_PUBLIC_CONFIG")
        list(APPEND __QtFeature_qmake_public_config "${arg_NAME}")
        set(__QtFeature_qmake_public_config "${__QtFeature_qmake_public_config}" PARENT_SCOPE)
    endif()
    if(arg_CONFIG_VAR_NAME STREQUAL "QMAKE_PRIVATE_CONFIG")
        list(APPEND __QtFeature_qmake_private_config "${arg_NAME}")
        set(__QtFeature_qmake_private_config "${__QtFeature_qmake_private_config}" PARENT_SCOPE)
    endif()
    if(arg_CONFIG_VAR_NAME STREQUAL "QMAKE_PUBLIC_QT_CONFIG")
        list(APPEND __QtFeature_qmake_public_qt_config "${arg_NAME}")
        set(__QtFeature_qmake_public_qt_config "${__QtFeature_qmake_public_qt_config}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_feature_definition feature name)
    qt_feature_normalize_name("${feature}" feature)
    cmake_parse_arguments(PARSE_ARGV 2 arg
        "NEGATE"
        "VALUE;PREREQUISITE"
        "")
    _qt_internal_validate_all_args_are_parsed(arg)

    # Store all the define related info in a unique variable key.
    set(key_name "_QT_FEATURE_DEFINE_DEFINITION_${feature}_${name}")
    set(${key_name} "FEATURE;${feature};NAME;${name};${ARGN}" PARENT_SCOPE)

    # Store the key for later evaluation and subsequent define generation:
    list(APPEND __QtFeature_define_definitions "${key_name}")

    set(__QtFeature_define_definitions ${__QtFeature_define_definitions} PARENT_SCOPE)
endfunction()

function(qt_evaluate_feature_definition key)
    if(NOT DEFINED ${key})
        qt_debug_print_variables(DEDUP MATCH "^_QT_FEATURE_DEFINE_DEFINITION")
        message(FATAL_ERROR "Attempting to evaluate feature define ${key} but its definition is missing. ")
    endif()

    cmake_parse_arguments(arg
        "NEGATE;"
        "FEATURE;NAME;VALUE;PREREQUISITE" "" ${${key}})

    set(expected ON)
    if (arg_NEGATE)
        set(expected OFF)
    endif()

    set(actual OFF)
    if(QT_FEATURE_${arg_FEATURE})
        set(actual ON)
    endif()

    set(msg "")

    if(actual STREQUAL expected)
        set(indent "")
        if(arg_PREREQUISITE)
            string(APPEND msg "#if ${arg_PREREQUISITE}\n")
            set(indent "  ")
        endif()
        if (arg_VALUE)
            string(APPEND msg "${indent}#define ${arg_NAME} ${arg_VALUE}\n")
        else()
            string(APPEND msg "${indent}#define ${arg_NAME}\n")
        endif()
        if(arg_PREREQUISITE)
            string(APPEND msg "#endif\n")
        endif()

        string(APPEND __QtFeature_public_extra "${msg}")
    endif()

    set(__QtFeature_public_extra ${__QtFeature_public_extra} PARENT_SCOPE)
endfunction()

function(qt_extra_definition name value)
    cmake_parse_arguments(PARSE_ARGV 2 arg
        "PUBLIC;PRIVATE"
        ""
        "")
    _qt_internal_validate_all_args_are_parsed(arg)

    if (arg_PUBLIC)
        string(APPEND __QtFeature_public_extra "\n#define ${name} ${value}\n")
    elseif(arg_PRIVATE)
        string(APPEND __QtFeature_private_extra "\n#define ${name} ${value}\n")
    endif()

    set(__QtFeature_public_extra ${__QtFeature_public_extra} PARENT_SCOPE)
    set(__QtFeature_private_extra ${__QtFeature_private_extra} PARENT_SCOPE)
endfunction()

function(qt_internal_generate_feature_line line feature)
    string(TOUPPER "${QT_FEATURE_${feature}}" value)
    if (value STREQUAL "ON")
        set(line "#define QT_FEATURE_${feature} 1\n\n" PARENT_SCOPE)
    elseif(value STREQUAL "OFF")
        set(line "#define QT_FEATURE_${feature} -1\n\n" PARENT_SCOPE)
    elseif(value STREQUAL "UNSET")
        set(line "#define QT_FEATURE_${feature} 0\n\n" PARENT_SCOPE)
    else()
        message(FATAL_ERROR "${feature} has unexpected value \"${QT_FEATURE_${feature}}\"! "
            "Valid values are ON, OFF and UNSET.")
    endif()
endfunction()

function(qt_internal_feature_write_file target file features extra)
    file(RELATIVE_PATH relative_path "${CMAKE_BINARY_DIR}" "${file}")

    string(MAKE_C_IDENTIFIER "${target}_${relative_path}" inclusion_guard_suffix)

    set(contents "")
    string(APPEND contents "#ifndef QT_FEATURES_${inclusion_guard_suffix}_H\n")
    string(APPEND contents "#define QT_FEATURES_${inclusion_guard_suffix}_H\n\n")
    foreach(it ${features})
        qt_internal_generate_feature_line(line "${it}")
        string(APPEND contents "${line}")
    endforeach()
    string(APPEND contents "${extra}\n")
    string(APPEND contents "#endif // QT_FEATURES_${inclusion_guard_suffix}_H\n")

    file(GENERATE OUTPUT "${file}" CONTENT "${contents}")
endfunction()

# Helper function which evaluates features from a given list of configure.cmake paths
# and creates the feature cache entries.
# Should not be used directly, unless features need to be available in a directory scope before the
# associated module evaluates the features.
# E.g. qtbase/src.pro needs access to Core features before src/corelib/CMakeLists.txt is parsed.
function(qt_feature_evaluate_features list_of_paths)
    qt_feature_module_begin(ONLY_EVALUATE_FEATURES)
    foreach(path ${list_of_paths})
        include("${path}")
    endforeach()
    qt_feature_module_end(ONLY_EVALUATE_FEATURES)
endfunction()

function(qt_feature_record_summary_entries list_of_paths)
    # Clean up any stale state just in case.
    qt_feature_unset_state_vars()

    set(__QtFeature_only_record_summary_entries TRUE)
    foreach(path ${list_of_paths})
        include("${path}")
    endforeach()
    qt_feature_unset_state_vars()
endfunction()

function(qt_feature_module_end)
    set(opt_args
        ONLY_EVALUATE_FEATURES
    )
    set(single_args
        OUT_VAR_PREFIX
    )
    set(multi_args "")
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")

    set(target ${arg_UNPARSED_ARGUMENTS})

    # The value of OUT_VAR_PREFIX is used as a prefix for output variables that should be
    # set in the parent scope.
    if(NOT arg_OUT_VAR_PREFIX)
        set(arg_OUT_VAR_PREFIX "")
    endif()

    set(all_features ${__QtFeature_public_features} ${__QtFeature_private_features} ${__QtFeature_internal_features})
    list(REMOVE_DUPLICATES all_features)

    foreach(feature ${all_features})
        qt_evaluate_feature(${feature})
    endforeach()

    # Evaluate custom cache assignments.
    foreach(cache_var_name ${__QtFeature_custom_enabled_cache_variables})
        set(${cache_var_name} ON CACHE BOOL "Force enabled by platform requirements." FORCE)
    endforeach()
    foreach(cache_var_name ${__QtFeature_custom_disabled_cache_variables})
        set(${cache_var_name} OFF CACHE BOOL "Force disabled by platform requirements." FORCE)
    endforeach()

    set(enabled_public_features "")
    set(disabled_public_features "")
    set(enabled_private_features "")
    set(disabled_private_features "")

    foreach(feature ${__QtFeature_public_features})
       if(QT_FEATURE_${feature})
          list(APPEND enabled_public_features ${feature})
       else()
          list(APPEND disabled_public_features ${feature})
       endif()
    endforeach()

    foreach(feature ${__QtFeature_private_features})
       if(QT_FEATURE_${feature})
          list(APPEND enabled_private_features ${feature})
       else()
          list(APPEND disabled_private_features ${feature})
       endif()
    endforeach()

    foreach(key ${__QtFeature_config_definitions})
        qt_evaluate_qmake_config_values(${key})
        unset(${key} PARENT_SCOPE)
    endforeach()

    foreach(key ${__QtFeature_define_definitions})
        qt_evaluate_feature_definition(${key})
        unset(${key} PARENT_SCOPE)
    endforeach()

    foreach(feature ${all_features})
        unset(_QT_FEATURE_DEFINITION_${feature} PARENT_SCOPE)
        unset(_QT_FEATURE_ALIASES_${feature} PARENT_SCOPE)
    endforeach()

    if(NOT arg_ONLY_EVALUATE_FEATURES AND NOT __QtFeature_no_headers)
        qt_internal_feature_write_file(${target}
            "${CMAKE_CURRENT_BINARY_DIR}/${__QtFeature_private_file}"
            "${__QtFeature_private_features}" "${__QtFeature_private_extra}"
        )

        qt_internal_feature_write_file(${target}
            "${CMAKE_CURRENT_BINARY_DIR}/${__QtFeature_public_file}"
            "${__QtFeature_public_features}" "${__QtFeature_public_extra}"
        )
    endif()

    if (NOT ("${target}" STREQUAL "NO_MODULE") AND NOT arg_ONLY_EVALUATE_FEATURES)
        get_target_property(targetType "${target}" TYPE)

        set(properties_to_export
            QT_ENABLED_PUBLIC_FEATURES
            QT_DISABLED_PUBLIC_FEATURES
            QT_ENABLED_PRIVATE_FEATURES
            QT_DISABLED_PRIVATE_FEATURES
            QT_QMAKE_PUBLIC_CONFIG
            QT_QMAKE_PRIVATE_CONFIG
            QT_QMAKE_PUBLIC_QT_CONFIG

        )
        if("${targetType}" STREQUAL "INTERFACE_LIBRARY")
            set(propertyPrefix "INTERFACE_")
            list(TRANSFORM properties_to_export PREPEND "${propertyPrefix}")
            # CMake doesn't allow us to export INTERFACE_* properties via EXPORT_PROPERTIES, it
            # says INTERFACE_* properties are reserved.
            # Instead, use our own property export infrastructure that places the values in the
            # module-specific Qt6<Foo>ExtraProperties.cmake file.
            # qt_internal_add_genex_properties_export was originally intended for properties with
            # genexes, but we can use it for this use case as well.
            # Before, we didn't use to export the properties at all for INTERFACE_ libraries,
            # but we need to, because certain GlobalPrivate modules have features which are used
            # in configure-time conditions for tests.
            qt_internal_add_custom_properties_to_export("${target}"
                PROPERTIES_WITHOUT_GENEXES
                    ${properties_to_export}
            )
        else()
            set(propertyPrefix "")
            set_property(TARGET "${target}"
                APPEND PROPERTY EXPORT_PROPERTIES ${properties_to_export})
        endif()

        foreach(visibility public private)
            string(TOUPPER "${visibility}" capitalVisibility)
            foreach(state enabled disabled)
                string(TOUPPER "${state}" capitalState)

                set_property(TARGET "${target}" PROPERTY ${propertyPrefix}QT_${capitalState}_${capitalVisibility}_FEATURES "${${state}_${visibility}_features}")
            endforeach()
        endforeach()

        set_property(TARGET "${target}"
                    PROPERTY ${propertyPrefix}QT_QMAKE_PUBLIC_CONFIG
                    "${__QtFeature_qmake_public_config}")
        set_property(TARGET "${target}"
                     PROPERTY ${propertyPrefix}QT_QMAKE_PRIVATE_CONFIG
                     "${__QtFeature_qmake_private_config}")
        set_property(TARGET "${target}"
                     PROPERTY ${propertyPrefix}QT_QMAKE_PUBLIC_QT_CONFIG
                     "${__QtFeature_qmake_public_qt_config}")

        # Config values were the old-school features before actual configure.json features were
        # implemented. Therefore "CONFIG+=foo" values should be considered features as well,
        # so that CMake can find them when building qtmultimedia for example.
        if(__QtFeature_qmake_public_config)
            set_property(TARGET "${target}"
                         APPEND PROPERTY ${propertyPrefix}QT_ENABLED_PUBLIC_FEATURES
                         ${__QtFeature_qmake_public_config})
        endif()
        if(__QtFeature_qmake_private_config)
            set_property(TARGET "${target}"
                         APPEND PROPERTY ${propertyPrefix}QT_ENABLED_PRIVATE_FEATURES
                         ${__QtFeature_qmake_private_config})
        endif()
        if(__QtFeature_qmake_public_qt_config)
            set_property(TARGET "${target}"
                         APPEND PROPERTY ${propertyPrefix}QT_ENABLED_PUBLIC_FEATURES
                         ${__QtFeature_qmake_public_qt_config})
        endif()

        qt_feature_copy_global_config_features_to_core(${target})
    endif()

    qt_feature_unset_state_vars()
endfunction()

macro(qt_feature_unset_state_vars)
    unset(__QtFeature_library PARENT_SCOPE)
    unset(__QtFeature_public_features PARENT_SCOPE)
    unset(__QtFeature_private_features PARENT_SCOPE)
    unset(__QtFeature_internal_features PARENT_SCOPE)

    unset(__QtFeature_private_file PARENT_SCOPE)
    unset(__QtFeature_public_file PARENT_SCOPE)
    unset(__QtFeature_no_headers PARENT_SCOPE)

    unset(__QtFeature_private_extra PARENT_SCOPE)
    unset(__QtFeature_public_extra PARENT_SCOPE)

    unset(__QtFeature_define_definitions PARENT_SCOPE)
    unset(__QtFeature_custom_enabled_features PARENT_SCOPE)
    unset(__QtFeature_custom_disabled_features PARENT_SCOPE)
    unset(__QtFeature_only_evaluate_features PARENT_SCOPE)
    unset(__QtFeature_only_record_summary_entries PARENT_SCOPE)
endmacro()

function(qt_feature_copy_global_config_features_to_core target)
    # CMake doesn't support setting custom properties on exported INTERFACE libraries
    # See https://gitlab.kitware.com/cmake/cmake/issues/19261.
    # To circumvent that, copy the properties from GlobalConfig to Core target.
    # This way the global features actually get set in the generated CoreTargets.cmake file.
    if(target STREQUAL Core)
        foreach(visibility public private)
            string(TOUPPER "${visibility}" capitalVisibility)
            foreach(state enabled disabled)
                string(TOUPPER "${state}" capitalState)

                set(core_property_name "QT_${capitalState}_${capitalVisibility}_FEATURES")
                set(global_property_name "INTERFACE_${core_property_name}")

                get_property(core_values TARGET Core PROPERTY ${core_property_name})
                get_property(global_values TARGET GlobalConfig PROPERTY ${global_property_name})

                set(total_values ${core_values} ${global_values})
                set_property(TARGET Core PROPERTY ${core_property_name} ${total_values})
            endforeach()
        endforeach()

        set(config_property_names
            QT_QMAKE_PUBLIC_CONFIG QT_QMAKE_PRIVATE_CONFIG QT_QMAKE_PUBLIC_QT_CONFIG )
        foreach(property_name ${config_property_names})
            set(core_property_name "${property_name}")
            set(global_property_name "INTERFACE_${core_property_name}")

            get_property(core_values TARGET Core PROPERTY ${core_property_name})
            get_property(global_values TARGET GlobalConfig PROPERTY ${global_property_name})

            set(total_values ${core_values} ${global_values})
            set_property(TARGET Core PROPERTY ${core_property_name} ${total_values})
        endforeach()
    endif()
endfunction()

function(qt_internal_detect_dirty_features)
    # We need to clean up QT_FEATURE_*, but only once per configuration cycle
    get_property(qt_feature_clean GLOBAL PROPERTY _qt_feature_clean)
    if(NOT qt_feature_clean AND NOT QT_NO_FEATURE_AUTO_RESET)
        message(STATUS "Checking for feature set changes")
        set_property(GLOBAL PROPERTY _qt_feature_clean TRUE)
        foreach(feature ${QT_KNOWN_FEATURES})
            if(DEFINED "FEATURE_${feature}" AND
                NOT "${QT_FEATURE_${feature}}" STREQUAL "${FEATURE_${feature}}")
                message("    '${feature}' was changed from ${QT_FEATURE_${feature}} "
                    "to ${FEATURE_${feature}}")
                set(dirty_build TRUE)
                set_property(GLOBAL APPEND PROPERTY _qt_dirty_features "${feature}")
            endif()
            unset("QT_FEATURE_${feature}" CACHE)
        endforeach()

        set(QT_KNOWN_FEATURES "" CACHE INTERNAL "" FORCE)

        if(dirty_build)
            set_property(GLOBAL PROPERTY _qt_dirty_build TRUE)
            message(WARNING
                "Due to detected feature set changes, dependent features "
                "will be re-computed automatically. This might cause a lot of files to be rebuilt. "
                "To disable this behavior, configure with -DQT_NO_FEATURE_AUTO_RESET=ON")
        endif()
    endif()
endfunction()

# Builds either a string of source code or a whole project to determine whether the build is
# successful.
#
# Sets a TEST_${name}_OUTPUT variable with the build output, to the scope of the calling function.
# Sets a TEST_${name} cache variable to either TRUE or FALSE if the build is successful or not.
#
# The test is only run if a feature condition needs to evaluate the TEST_${name} variable. If you
# need the test result regardless of any feature conditions, call
# qt_run_config_compile_test right after qt_config_compile_test.
macro(qt_config_compile_test name)
    set_property(GLOBAL APPEND PROPERTY _qtfeature_known_compile_tests ${name})
    set_property(GLOBAL PROPERTY _qtfeature_compile_test_args_${name} ${ARGN})
    if(QT_RUN_COMPILE_TESTS_IMMEDIATELY)
        qt_run_config_compile_test(${name})
    endif()
endmacro()

# Runs a compile test that was defined with qt_config_compile_test.
function(qt_run_config_compile_test name)
    if(DEFINED "TEST_${name}")
        return()
    endif()

    get_property(test_args GLOBAL PROPERTY _qtfeature_compile_test_args_${name})
    if("${test_args}" STREQUAL "")
        message(FATAL_ERROR
            "Can't find definition for compile test '${name}'. "
            "The test probably wasn't defined with qt_config_compile_test."
        )
    endif()
    cmake_parse_arguments(arg "" "LABEL;PROJECT_PATH;C_STANDARD;CXX_STANDARD"
        "COMPILE_OPTIONS;LIBRARIES;CODE;PACKAGES;CMAKE_FLAGS" ${test_args})

    if(arg_PROJECT_PATH)
        message(STATUS "Performing Test ${arg_LABEL}")

        set(flags "")
        qt_get_platform_try_compile_vars(platform_try_compile_vars)
        list(APPEND flags ${platform_try_compile_vars})

        # If the repo has its own cmake modules, include those in the module path, so that various
        # find_package calls work.
        if(EXISTS "${PROJECT_SOURCE_DIR}/cmake")
            set(must_append_module_path_flag TRUE)
            set(flags_copy "${flags}")
            set(flags)
            foreach(flag IN LISTS flags_copy)
                if(flag MATCHES "^-DCMAKE_MODULE_PATH:STRING=")
                    set(must_append_module_path_flag FALSE)
                    set(flag "${flag}\\;${PROJECT_SOURCE_DIR}/cmake")
                endif()
                list(APPEND flags "${flag}")
            endforeach()
            if(must_append_module_path_flag)
                list(APPEND flags "-DCMAKE_MODULE_PATH:STRING=${PROJECT_SOURCE_DIR}/cmake")
            endif()
        endif()

        # Pass which packages need to be found.
        if(arg_PACKAGES)
            set(packages_list "")

            # Parse the package names, version, etc. An example would be:
            # PACKAGE Foo 6 REQUIRED
            # PACKAGE Bar 2 COMPONENTS Baz
            foreach(p ${arg_PACKAGES})
                if(p STREQUAL PACKAGE)
                    if(package_entry)
                        # Encode the ";" into "\;" to separate the arguments of a find_package call.
                        string(REPLACE ";" "\\;" package_entry_string "${package_entry}")
                        list(APPEND packages_list "${package_entry_string}")
                    endif()

                    set(package_entry "")
                else()
                    list(APPEND package_entry "${p}")
                endif()
            endforeach()
            # Parse final entry.
            if(package_entry)
                string(REPLACE ";" "\\;" package_entry_string "${package_entry}")
                list(APPEND packages_list "${package_entry_string}")
            endif()

            # Encode the ";" again.
            string(REPLACE ";" "\\;" packages_list "${packages_list}")

            # The flags are separated by ';', the find_package entries by '\;',
            # and the package parts of an entry by '\\;'.
            # Example:
            # WrapFoo\\;6\\;COMPONENTS\\;bar\;WrapBaz\\;5
            list(APPEND flags "-DQT_CONFIG_COMPILE_TEST_PACKAGES:STRING=${packages_list}")

            # Inside the project, the value of QT_CONFIG_COMPILE_TEST_PACKAGES is used in a foreach
            # loop that calls find_package() for each package entry, and thus the variable expansion
            # ends up calling something like find_package(WrapFoo;6;COMPONENTS;bar) aka
            # find_package(WrapFoo 6 COMPONENTS bar).
        endif()

        # Pass which libraries need to be linked against.
        if(arg_LIBRARIES)
            set(link_flags "")
            set(library_targets "")
            # Separate targets from link flags or paths. This is to prevent configuration failures
            # when the targets are not found due to missing packages.
            foreach(lib ${arg_LIBRARIES})
                string(FIND "${lib}" "::" is_library_target)
                if(is_library_target EQUAL -1)
                    list(APPEND link_flags "${lib}")
                else()
                    list(APPEND library_targets "${lib}")
                endif()
            endforeach()
            if(link_flags)
                list(APPEND flags "-DQT_CONFIG_COMPILE_TEST_LIBRARIES:STRING=${link_flags}")
            endif()
            if(library_targets)
                list(APPEND flags
                     "-DQT_CONFIG_COMPILE_TEST_LIBRARY_TARGETS:STRING=${library_targets}")
            endif()
        endif()

        # Pass override values for CMAKE_SYSTEM_{PREFIX|FRAMEWORK}_PATH.
        if(DEFINED QT_CMAKE_SYSTEM_PREFIX_PATH_BACKUP)
            set(path_list ${CMAKE_SYSTEM_PREFIX_PATH})
            string(REPLACE ";" "\\;" path_list "${path_list}")
            list(APPEND flags "-DQT_CONFIG_COMPILE_TEST_CMAKE_SYSTEM_PREFIX_PATH=${path_list}")
        endif()
        if(DEFINED QT_CMAKE_SYSTEM_FRAMEWORK_PATH_BACKUP)
            set(path_list ${CMAKE_SYSTEM_FRAMEWORK_PATH})
            string(REPLACE ";" "\\;" path_list "${path_list}")
            list(APPEND flags "-DQT_CONFIG_COMPILE_TEST_CMAKE_SYSTEM_FRAMEWORK_PATH=${path_list}")
        endif()

        if(NOT arg_CMAKE_FLAGS)
            set(arg_CMAKE_FLAGS "")
        endif()

        # CI passes the project dir of the Qt repository as absolute path without drive letter:
        #   \Users\qt\work\qt\qtbase
        # Ensure that arg_PROJECT_PATH is an absolute path with drive letter:
        #   C:/Users/qt/work/qt/qtbase
        # This works around CMake upstream issue #22534.
        if(CMAKE_HOST_WIN32)
            get_filename_component(arg_PROJECT_PATH "${arg_PROJECT_PATH}" REALPATH)
        endif()

        try_compile(
            HAVE_${name} "${CMAKE_BINARY_DIR}/config.tests/${name}" "${arg_PROJECT_PATH}" "${name}"
            CMAKE_FLAGS ${flags} ${arg_CMAKE_FLAGS}
            OUTPUT_VARIABLE try_compile_output
        )

        if(${HAVE_${name}})
            set(status_label "Success")
        else()
            set(status_label "Failed")
        endif()
        message(STATUS "Performing Test ${arg_LABEL} - ${status_label}")
    else()
        foreach(library IN ITEMS ${arg_LIBRARIES})
            if(NOT TARGET "${library}")
                # If the dependency looks like a cmake target, then make this compile test
                # fail instead of cmake abort later via CMAKE_REQUIRED_LIBRARIES.
                string(FIND "${library}" "::" cmake_target_namespace_separator)
                if(NOT cmake_target_namespace_separator EQUAL -1)
                    message(STATUS "Performing Test ${arg_LABEL} - Failed because ${library} not found")
                    set(HAVE_${name} FALSE)
                    break()
                endif()
            endif()
        endforeach()

        if(NOT DEFINED HAVE_${name})
            set(_save_CMAKE_C_STANDARD "${CMAKE_C_STANDARD}")
            set(_save_CMAKE_C_STANDARD_REQUIRED "${CMAKE_C_STANDARD_REQUIRED}")
            set(_save_CMAKE_CXX_STANDARD "${CMAKE_CXX_STANDARD}")
            set(_save_CMAKE_CXX_STANDARD_REQUIRED "${CMAKE_CXX_STANDARD_REQUIRED}")
            set(_save_CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
            set(_save_CMAKE_TRY_COMPILE_PLATFORM_VARIABLES "${CMAKE_TRY_COMPILE_PLATFORM_VARIABLES}")

            if(arg_C_STANDARD)
               set(CMAKE_C_STANDARD "${arg_C_STANDARD}")
               set(CMAKE_C_STANDARD_REQUIRED OFF)
            endif()

            if(arg_CXX_STANDARD)
                if((${arg_CXX_STANDARD} LESS 23 OR ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.20") AND
                   (${arg_CXX_STANDARD} LESS 26 OR ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.25"))
                    set(CMAKE_CXX_STANDARD "${arg_CXX_STANDARD}")
                    set(CMAKE_CXX_STANDARD_REQUIRED OFF)
                endif()
            endif()

            set(CMAKE_REQUIRED_FLAGS ${arg_COMPILE_OPTIONS})

            # Pass -stdlib=libc++ on if necessary
            if (QT_FEATURE_stdlib_libcpp)
                list(APPEND CMAKE_REQUIRED_FLAGS "-stdlib=libc++")
            endif()

            # For MSVC we need to explicitly pass -Zc:__cplusplus to get correct __cplusplus
            # define values. According to common/msvc-version.conf the flag is supported starting
            # with 1913.
            # https://developercommunity.visualstudio.com/content/problem/139261/msvc-incorrectly-defines-cplusplus.html
            # No support for the flag in upstream CMake as of 3.17.
            # https://gitlab.kitware.com/cmake/cmake/issues/18837
            if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND MSVC_VERSION GREATER_EQUAL 1913)
                list(APPEND CMAKE_REQUIRED_FLAGS "-Zc:__cplusplus")
            endif()

            # Let CMake load our custom platform modules.
            if(NOT QT_AVOID_CUSTOM_PLATFORM_MODULES)
                list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES CMAKE_MODULE_PATH)
            endif()

            set(_save_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
            set(CMAKE_REQUIRED_LIBRARIES "${arg_LIBRARIES}")

            _qt_internal_get_check_cxx_source_compiles_out_var(try_compile_output extra_args)
            check_cxx_source_compiles(
                "${arg_UNPARSED_ARGUMENTS} ${arg_CODE}" HAVE_${name} ${extra_args}
            )
            set(CMAKE_REQUIRED_LIBRARIES "${_save_CMAKE_REQUIRED_LIBRARIES}")

            set(CMAKE_C_STANDARD "${_save_CMAKE_C_STANDARD}")
            set(CMAKE_C_STANDARD_REQUIRED "${_save_CMAKE_C_STANDARD_REQUIRED}")
            set(CMAKE_CXX_STANDARD "${_save_CMAKE_CXX_STANDARD}")
            set(CMAKE_CXX_STANDARD_REQUIRED "${_save_CMAKE_CXX_STANDARD_REQUIRED}")
            set(CMAKE_REQUIRED_FLAGS "${_save_CMAKE_REQUIRED_FLAGS}")
            set(CMAKE_TRY_COMPILE_PLATFORM_VARIABLES "${_save_CMAKE_TRY_COMPILE_PLATFORM_VARIABLES}")
        endif()
    endif()

    # Note this is assigned to the parent scope, and is not a CACHE var, which means the value is
    # only available on first configuration.
    set(TEST_${name}_OUTPUT "${try_compile_output}" PARENT_SCOPE)

    # Story the compile output for a test in a global property. It will only be available on first
    # configuration, because we don't cache it across cmake invocations.
    set_property(GLOBAL PROPERTY _qt_run_config_compile_test_output_${name} "${try_compile_output}")

    set(TEST_${name} "${HAVE_${name}}" CACHE INTERNAL "${arg_LABEL}")
endfunction()

# This function should be used for passing required try compile platform variables to the
# project-based try_compile() call.
# out_var will be a list of -Dfoo=bar strings, suitable to pass to CMAKE_FLAGS.
function(qt_get_platform_try_compile_vars out_var)
    # Use the regular variables that are used for source-based try_compile() calls.
    set(flags "${CMAKE_TRY_COMPILE_PLATFORM_VARIABLES}")

    # Pass custom flags.
    list(APPEND flags "CMAKE_C_FLAGS")
    list(APPEND flags "CMAKE_C_FLAGS_DEBUG")
    list(APPEND flags "CMAKE_C_FLAGS_RELEASE")
    list(APPEND flags "CMAKE_C_FLAGS_RELWITHDEBINFO")
    list(APPEND flags "CMAKE_CXX_FLAGS")
    list(APPEND flags "CMAKE_CXX_FLAGS_DEBUG")
    list(APPEND flags "CMAKE_CXX_FLAGS_RELEASE")
    list(APPEND flags "CMAKE_CXX_FLAGS_RELWITHDEBINFO")
    list(APPEND flags "CMAKE_OBJCOPY")
    list(APPEND flags "CMAKE_EXE_LINKER_FLAGS")

    # Pass toolchain files.
    if(CMAKE_TOOLCHAIN_FILE)
        list(APPEND flags "CMAKE_TOOLCHAIN_FILE")
    endif()
    if(VCPKG_CHAINLOAD_TOOLCHAIN_FILE)
        list(APPEND flags "VCPKG_CHAINLOAD_TOOLCHAIN_FILE")
    endif()

    # Pass language standard flags.
    list(APPEND flags "CMAKE_C_STANDARD")
    list(APPEND flags "CMAKE_C_STANDARD_REQUIRED")
    list(APPEND flags "CMAKE_CXX_STANDARD")
    list(APPEND flags "CMAKE_CXX_STANDARD_REQUIRED")

    # Pass -stdlib=libc++ on if necessary
    if (QT_FEATURE_stdlib_libcpp)
        if(CMAKE_CXX_FLAGS)
            string(APPEND CMAKE_CXX_FLAGS " -stdlib=libc++")
        else()
            set(CMAKE_CXX_FLAGS "-stdlib=libc++")
        endif()
    endif()

    # Assemble the list with regular options.
    set(flags_cmd_line "")
    foreach(flag ${flags})
        if(${flag})
            list(APPEND flags_cmd_line "-D${flag}=${${flag}}")
        endif()
    endforeach()

    # Let CMake load our custom platform modules.
    if(NOT QT_AVOID_CUSTOM_PLATFORM_MODULES)
        list(APPEND flags_cmd_line "-DCMAKE_MODULE_PATH:STRING=${QT_CMAKE_DIR}/platforms")
    endif()

    # Pass darwin specific options.
    # The architectures need to be passed explicitly to project-based try_compile calls even on
    # macOS, so that arm64 compilation works on Apple silicon.
    qt_internal_get_first_osx_arch(osx_first_arch)
    if(osx_first_arch)
        # Do what qmake does, aka when doing a simulator_and_device build, build the
        # target architecture test only with the first given architecture, which should be the
        # device architecture, aka some variation of "arm" (armv7, arm64).
        list(APPEND flags_cmd_line "-DCMAKE_OSX_ARCHITECTURES:STRING=${osx_first_arch}")
    endif()
    if(UIKIT)
        # Specify the sysroot, but only if not doing a simulator_and_device build.
        # So keep the sysroot empty for simulator_and_device builds.
        if(QT_APPLE_SDK)
            list(APPEND flags_cmd_line "-DCMAKE_OSX_SYSROOT:STRING=${QT_APPLE_SDK}")
        endif()
    endif()
    if(QT_NO_USE_FIND_PACKAGE_SYSTEM_ENVIRONMENT_PATH)
        list(APPEND flags_cmd_line "-DCMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH:BOOL=OFF")
    endif()

    if(ANDROID)
        qt_internal_get_android_cmake_policy_version_minimum_assignment(
            android_cmake_policy_version_minimum TYPE COMMAND_LINE)
        if(android_cmake_policy_version_minimum)
            list(APPEND flags_cmd_line "${android_cmake_policy_version_minimum}")
        endif()
    endif()

    set("${out_var}" "${flags_cmd_line}" PARENT_SCOPE)
endfunction()

# Set out_var to the first value of CMAKE_OSX_ARCHITECTURES.
# Sets an empty string if no architecture is present.
function(qt_internal_get_first_osx_arch out_var)
    set(value "")
    if(CMAKE_OSX_ARCHITECTURES)
        list(GET CMAKE_OSX_ARCHITECTURES 0 value)
    endif()
    set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(qt_config_compile_test_x86simd extension label)
    if (DEFINED TEST_X86SIMD_${extension})
        return()
    endif()

    set(flags "-DSIMD:string=${extension}")

    qt_get_platform_try_compile_vars(platform_try_compile_vars)
    list(APPEND flags ${platform_try_compile_vars})

    message(STATUS "Performing Test ${label} intrinsics")
    try_compile("TEST_X86SIMD_${extension}"
        "${CMAKE_CURRENT_BINARY_DIR}/config.tests/x86_simd_${extension}"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/x86_simd"
        x86_simd
        CMAKE_FLAGS ${flags})
    if(${TEST_X86SIMD_${extension}})
        set(status_label "Success")
    else()
        set(status_label "Failed")
    endif()
    message(STATUS "Performing Test ${label} intrinsics - ${status_label}")
    set(TEST_subarch_${extension} "${TEST_X86SIMD_${extension}}" CACHE INTERNAL "${label}")
endfunction()

function(qt_config_compile_test_armintrin extension label)
    if (DEFINED TEST_ARMINTRIN_${extension})
        return()
    endif()

    set(flags "-DSIMD:string=${extension}")

    qt_get_platform_try_compile_vars(platform_try_compile_vars)
    list(APPEND flags ${platform_try_compile_vars})

    message(STATUS "Performing Test ${label} intrinsics")
    try_compile("TEST_ARMINTRIN_${extension}"
        "${CMAKE_CURRENT_BINARY_DIR}/config.tests/armintrin_${extension}"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/armintrin"
        armintrin
        CMAKE_FLAGS ${flags})
    if(${TEST_ARMINTRIN_${extension}})
        set(status_label "Success")
    else()
        set(status_label "Failed")
    endif()
    message(STATUS "Performing Test ${label} intrinsics - ${status_label}")
    set(TEST_subarch_${extension} "${TEST_ARMINTRIN_${extension}}" CACHE INTERNAL "${label}")
endfunction()

function(qt_config_compile_test_loongarchsimd extension label)
        if (DEFINED TEST_LOONGARCHSIMD_${extension})
        return()
    endif()

    set(flags "-DSIMD:string=${extension}")

    qt_get_platform_try_compile_vars(platform_try_compile_vars)
    list(APPEND flags ${platform_try_compile_vars})

    message(STATUS "Performing Test ${label} intrinsics")
    try_compile("TEST_LOONGARCHSIMD_${extension}"
        "${CMAKE_CURRENT_BINARY_DIR}/config.tests/loongarch_simd_${extension}"
        "${CMAKE_CURRENT_SOURCE_DIR}/config.tests/loongarch_simd"
        loongarch_simd
        CMAKE_FLAGS ${flags})
    if(${TEST_LOONGARCHSIMD_${extension}})
        set(status_label "Success")
    else()
        set(status_label "Failed")
    endif()
    message(STATUS "Performing Test ${label} intrinsics - ${status_label}")
    set(TEST_subarch_${extension} "${TEST_LOONGARCHSIMD_${extension}}" CACHE INTERNAL "${label}")
endfunction()

function(qt_config_compile_test_machine_tuple label)
    if(DEFINED TEST_MACHINE_TUPLE OR NOT (LINUX OR HURD) OR ANDROID)
        return()
    endif()

    message(STATUS "Performing Test ${label}")
    execute_process(COMMAND "${CMAKE_CXX_COMPILER}" -dumpmachine
        OUTPUT_VARIABLE output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE exit_code)
    if(exit_code EQUAL 0)
        set(status_label "Success")
    else()
        set(status_label "Failed")
    endif()
    message(STATUS "Performing Test ${label} - ${status_label}")
    set(TEST_machine_tuple "${output}" CACHE INTERNAL "${label}")
endfunction()

function(qt_config_compiler_supports_flag_test name)
    if(DEFINED "TEST_${name}")
        return()
    endif()

    cmake_parse_arguments(arg "" "LABEL;FLAG" "" ${ARGN})
    check_cxx_compiler_flag("${arg_FLAG}" TEST_${name})
    set(TEST_${name} "${TEST_${name}}" CACHE INTERNAL "${label}")
endfunction()

# gcc expects -fuse-ld=mold (no absolute path can be given) (gcc >= 12.1)
#             or an 'ld' symlink to 'mold' in a dir that is passed via -B flag (gcc < 12.1)
#
# clang expects     -fuse-ld=mold
#                or -fuse-ld=<mold-abs-path>
#                or --ldpath=<mold-abs-path>  (clang >= 12)
# https://github.com/rui314/mold/#how-to-use
# TODO: In the gcc < 12.1 case, the qt_internal_check_if_linker_is_available(mold) check will
#       always return TRUE because gcc will not error out if it is given a -B flag pointing to an
#       invalid dir, as well as when the the symlink to the linker in the -B dir is not actually
#       a valid linker.
#       It would be nice to handle that case in a better way, but it's not that important
#       given that gcc > 12.1 now supports -fuse-ld=mold
# NOTE: In comparison to clang, in the gcc < 12.1 case, we pass the full path to where mold is
#       and that is recorded in PlatformCommonInternal's INTERFACE_LINK_OPTIONS target.
#       Moving such a Qt to a different machine and trying to build another repo won't
#       work because the recorded path will be invalid. This is not a problem with
#       the gcc >= 12.1 case
function(qt_internal_get_mold_linker_flags out_var)
    cmake_parse_arguments(PARSE_ARGV 1 arg "ERROR_IF_EMPTY" "" "")

    find_program(QT_INTERNAL_LINKER_MOLD mold)

    set(flag "")
    if(QT_INTERNAL_LINKER_MOLD)
        if(GCC)
            if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.1")
                set(flag "-fuse-ld=mold")
            else()
                set(mold_linker_dir "${CMAKE_CURRENT_BINARY_DIR}/.qt_linker")
                set(mold_linker_path "${mold_linker_dir}/ld")
                if(NOT EXISTS "${mold_linker_dir}")
                    file(MAKE_DIRECTORY "${mold_linker_dir}")
                endif()
                if(NOT EXISTS "${mold_linker_path}")
                    file(CREATE_LINK
                        "${QT_INTERNAL_LINKER_MOLD}"
                        "${mold_linker_path}"
                         SYMBOLIC)
                endif()
                set(flag "-B${mold_linker_dir}")
            endif()
        elseif(CLANG)
            if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12")
                set(flag "--ld-path=mold")
            else()
                set(flag "-fuse-ld=mold")
            endif()
        endif()
    endif()
    if(arg_ERROR_IS_EMPTY AND NOT flag)
        message(FATAL_ERROR "Could not determine the flags to use the mold linker.")
    endif()
    set(${out_var} "${flag}" PARENT_SCOPE)
endfunction()

function(qt_internal_get_active_linker_flags out_var)
    set(flags "")
    if(GCC OR CLANG)
        if(QT_FEATURE_use_gold_linker)
            list(APPEND flags "-fuse-ld=gold")
        elseif(QT_FEATURE_use_bfd_linker)
            list(APPEND flags "-fuse-ld=bfd")
        elseif(QT_FEATURE_use_lld_linker)
            list(APPEND flags "-fuse-ld=lld")
        elseif(QT_FEATURE_use_mold_linker)
            qt_internal_get_mold_linker_flags(mold_flags ERROR_IF_EMPTY)
            list(APPEND flags "${mold_flags}")
        endif()
    endif()
    set(${out_var} "${flags}" PARENT_SCOPE)
endfunction()

function(qt_internal_check_if_linker_is_available name)
    if(DEFINED "TEST_${name}")
        return()
    endif()

    cmake_parse_arguments(arg "" "LABEL;FLAG" "" ${ARGN})
    set(flags "${arg_FLAG}")

    set(CMAKE_REQUIRED_LINK_OPTIONS ${flags})
    check_cxx_source_compiles("int main() { return 0; }" TEST_${name})
    set(TEST_${name} "${TEST_${name}}" CACHE INTERNAL "${label}")
endfunction()

function(qt_config_linker_supports_flag_test name)
    if(DEFINED "TEST_${name}")
        return()
    endif()

    cmake_parse_arguments(arg "" "LABEL;FLAG" "" ${ARGN})
    if(GCC OR CLANG)
        set(flags "-Wl,--fatal-warnings,${arg_FLAG}")
    elseif(MSVC)
        set(flags "${arg_FLAG}")
    else()
        # We don't know how to pass linker options in a way that
        # it reliably fails, so assume the detection failed.
        set(TEST_${name} "0" CACHE INTERNAL "${label}")
        return()
    endif()

    # Pass the linker that the main project uses to the compile test.
    qt_internal_get_active_linker_flags(linker_flags)
    if(linker_flags)
        list(PREPEND flags ${linker_flags})
    endif()

    set(CMAKE_REQUIRED_LINK_OPTIONS ${flags})
    check_cxx_source_compiles("int main() { return 0; }" TEST_${name})
    set(TEST_${name} "${TEST_${name}}" CACHE INTERNAL "${label}")
endfunction()

function(qt_make_features_available target)
    if(NOT "${target}" MATCHES "^${QT_CMAKE_EXPORT_NAMESPACE}::[a-zA-Z0-9_-]*$")
        message(FATAL_ERROR "${target} does not match ${QT_CMAKE_EXPORT_NAMESPACE}::[a-zA-Z0-9_-]*. INVALID NAME.")
    endif()
    if(NOT TARGET ${target})
        message(FATAL_ERROR "${target} not found.")
    endif()

    get_target_property(target_type "${target}" TYPE)
    if("${target_type}" STREQUAL "INTERFACE_LIBRARY")
        set(property_prefix "INTERFACE_")
    else()
        set(property_prefix "")
    endif()
    foreach(visibility IN ITEMS PUBLIC PRIVATE)
        set(value ON)
        foreach(state IN ITEMS ENABLED DISABLED)
            get_target_property(features "${target}" ${property_prefix}QT_${state}_${visibility}_FEATURES)
            if("${features}" STREQUAL "features-NOTFOUND")
                continue()
            endif()
            foreach(feature IN ITEMS ${features})
                if (DEFINED "QT_FEATURE_${feature}" AND NOT "${QT_FEATURE_${feature}}" STREQUAL "${value}")
                    message(WARNING
                        "This project was initially configured with the Qt feature \"${feature}\" "
                        "set to \"${QT_FEATURE_${feature}}\". While loading the "
                        "\"${target}\" package, the value of the feature "
                        "has changed to \"${value}\". That might cause a project rebuild due to "
                        "updated C++ headers. \n"
                        "In case of build issues, consider removing the CMakeCache.txt file and "
                        "reconfiguring the project."
                    )
                endif()
                set(QT_FEATURE_${feature} "${value}" CACHE INTERNAL "Qt feature: ${feature} (from target ${target})")
            endforeach()
            set(value OFF)
        endforeach()
    endforeach()
endfunction()
