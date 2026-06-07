# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt_internal_set_message_log_level out_var)
    # Decide whether output should be verbose or not.
    # Default to verbose (--log-level=STATUS) in a developer-build and
    # non-verbose (--log-level=NOTICE) otherwise.
    # If a custom CMAKE_MESSAGE_LOG_LEVEL was specified, it takes priority.
    # Passing an explicit --log-level=Foo has the highest priority.
    if(NOT CMAKE_MESSAGE_LOG_LEVEL)
        if(FEATURE_developer_build OR QT_FEATURE_developer_build)
            set(CMAKE_MESSAGE_LOG_LEVEL "STATUS")
        else()
            set(CMAKE_MESSAGE_LOG_LEVEL "NOTICE")
        endif()
        set(${out_var} "${CMAKE_MESSAGE_LOG_LEVEL}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_print_feature_summary)
    if(QT_SUPERBUILD)
        qt_internal_set_message_log_level(message_log_level)
        if(message_log_level)
            # In a top-level build, ensure that the feature_summary is affected by the
            # selected log-level.
            set(CMAKE_MESSAGE_LOG_LEVEL "${message_log_level}")
        endif()
    endif()

    # Print debug information about which tests were not run.
    get_property(known_compile_tests GLOBAL PROPERTY _qtfeature_known_compile_tests)
    list(REMOVE_DUPLICATES known_compile_tests)
    set(tests_not_run "")
    foreach(test_name IN LISTS known_compile_tests)
        if(NOT DEFINED "TEST_${test_name}")
            list(APPEND tests_not_run "${test_name}")
        endif()
    endforeach()
    if(tests_not_run STREQUAL "")
        message(DEBUG "All known compile tests were run.")
    else()
        message(DEBUG
            "The following compile tests were not run, because their values were not requested."
        )
        foreach(test_name IN LISTS tests_not_run)
            message(DEBUG "The compile test '${test_name}' was not run.")
        endforeach()
    endif()

    # Print an SBOM section for a top-level build, or a single repo.
    if(NOT QT_NO_SBOM_SUMMARY_INFO)
        qt_internal_add_sbom_summary_info()
    endif()

    # Show which packages were found.
    feature_summary(INCLUDE_QUIET_PACKAGES
                    WHAT PACKAGES_FOUND
                         REQUIRED_PACKAGES_NOT_FOUND
                         RECOMMENDED_PACKAGES_NOT_FOUND
                         OPTIONAL_PACKAGES_NOT_FOUND
                         RUNTIME_PACKAGES_NOT_FOUND
                         FATAL_ON_MISSING_REQUIRED_PACKAGES)
    qt_internal_run_additional_summary_checks()
    qt_configure_print_summary()
endfunction()

function(qt_internal_run_additional_summary_checks)
    get_property(
        rpath_workaround_enabled
        GLOBAL PROPERTY _qt_internal_staging_prefix_build_rpath_workaround)
    if(rpath_workaround_enabled)
        set(message
            "Due to CMAKE_STAGING_PREFIX usage and an unfixed CMake bug,
      to ensure correct build time rpaths, directory-level install
      rules like ninja src/gui/install will not work.
      Check QTBUG-102592 for further details.")
        qt_configure_add_report_entry(
            TYPE NOTE
            MESSAGE "${message}"
        )
    endif()
endfunction()

function(qt_print_build_instructions)
    if((NOT PROJECT_NAME STREQUAL "QtBase" AND
        NOT PROJECT_NAME STREQUAL "Qt") OR QT_INTERNAL_BUILD_STANDALONE_PARTS)

        return()
    endif()

    if(QT_SUPERBUILD)
        qt_internal_set_message_log_level(message_log_level)
        if(message_log_level)
            # In a top-level build, ensure that qt_print_build_instructions is affected by the
            # selected log-level.
            set(CMAKE_MESSAGE_LOG_LEVEL "${message_log_level}")
        endif()
    endif()

    set(build_command "cmake --build . --parallel")
    set(install_command "cmake --install .")

    # Suggest "ninja install" for Multi-Config builds
    # until https://gitlab.kitware.com/cmake/cmake/-/issues/21475 is fixed.
    if(CMAKE_GENERATOR STREQUAL "Ninja Multi-Config")
        set(install_command "ninja install")
    endif()

    set(configure_module_command "qt-configure-module")
    if(CMAKE_HOST_WIN32)
        string(APPEND configure_module_command ".bat")
    endif()
    if("${CMAKE_STAGING_PREFIX}" STREQUAL "")
        set(local_install_prefix "${CMAKE_INSTALL_PREFIX}")
    else()
        set(local_install_prefix "${CMAKE_STAGING_PREFIX}")
    endif()

    set(msg "\n")

    list(APPEND msg "Qt is now configured for building. Just run '${build_command}'\n")
    if(QT_WILL_INSTALL)
        list(APPEND msg "Once everything is built, you must run '${install_command}'")
        list(APPEND msg "Qt will be installed into '${CMAKE_INSTALL_PREFIX}'")
    else()
        list(APPEND msg
            "Once everything is built, Qt is installed. You should NOT run '${install_command}'")
        list(APPEND msg
            "Note that this build cannot be deployed to other machines or devices.")
    endif()
    list(APPEND msg
        "\nTo configure and build other Qt modules, you can use the following convenience script:
        ${local_install_prefix}/${INSTALL_BINDIR}/${configure_module_command}")
    list(APPEND msg "\nIf reconfiguration fails for some reason, try removing 'CMakeCache.txt' \
from the build directory")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24")
        list(APPEND msg "Alternatively, you can add the --fresh flag to your CMake flags.\n")
    else()
        list(APPEND msg "\n")
    endif()

    list(JOIN msg "\n" msg)

    if(NOT QT_INTERNAL_BUILD_INSTRUCTIONS_SHOWN)
        qt_configure_print_build_instructions_helper("${msg}")
    endif()

    set(QT_INTERNAL_BUILD_INSTRUCTIONS_SHOWN "TRUE" CACHE STRING "" FORCE)

    if(QT_SUPERBUILD)
        qt_internal_save_previously_visited_packages()
    endif()
endfunction()

function(qt_configure_print_summary_helper summary_reports force_show)
    # We force show the summary by temporarily (within the scope of the function) resetting the
    # current log level.
    if(force_show)
        set(CMAKE_MESSAGE_LOG_LEVEL "STATUS")

        # Need 2 flushes to ensure no interleaved input is printed due to a mix of message(STATUS)
        # and message(NOTICE) calls.
        execute_process(COMMAND ${CMAKE_COMMAND} -E echo " ")

        message(STATUS "Configure summary:\n${summary_reports}")

        execute_process(COMMAND ${CMAKE_COMMAND} -E echo " ")
    endif()
endfunction()

function(qt_configure_print_build_instructions_helper msg)
    # We want to ensure build instructions are always shown the first time, regardless of the
    # current log level.
    set(CMAKE_MESSAGE_LOG_LEVEL "STATUS")
    message(STATUS "${msg}")
endfunction()

function(qt_configure_print_summary)
    # Evaluate all recorded commands.
    qt_configure_eval_commands()

    set(summary_file "${CMAKE_BINARY_DIR}/config.summary")
    file(WRITE "${summary_file}" "")

    get_property(features_possibly_changed GLOBAL PROPERTY _qt_dirty_build)

    # Show Qt-specific configuration summary.
    if(__qt_configure_reports)
        # The summary will only be printed for log level STATUS or above.
        # Check whether the log level is sufficient for printing the summary.
        set(log_level_sufficient_for_printed_summary TRUE)
        if(CMAKE_VERSION GREATER_EQUAL "3.25")
            cmake_language(GET_MESSAGE_LOG_LEVEL log_level)
            set(sufficient_log_levels STATUS VERBOSE DEBUG TRACE)
            if(NOT log_level IN_LIST sufficient_log_levels)
                set(log_level_sufficient_for_printed_summary FALSE)
            endif()
        endif()

        # We want to show the configuration summary file and log level message only on
        # first configuration or when we detect a feature change, to keep most
        # reconfiguration output as quiet as possible.
        # Currently feature change detection is not entirely reliable.
        if(log_level_sufficient_for_printed_summary
                AND (NOT QT_INTERNAL_SUMMARY_INSTRUCTIONS_SHOWN OR features_possibly_changed))
            set(force_show_summary TRUE)
            message(
                "\n"
                "-- Configuration summary shown below. It has also been written to"
                " ${CMAKE_BINARY_DIR}/config.summary")
            message(
                "-- Configure with --log-level=STATUS or higher to increase "
                "CMake's message verbosity. "
                "The log level does not persist across reconfigurations.")
        else()
            set(force_show_summary FALSE)
            message(
                "\n"
                "-- Configuration summary has been written to"
                " ${CMAKE_BINARY_DIR}/config.summary")
        endif()

        qt_configure_print_summary_helper(
            "${__qt_configure_reports}"
            ${force_show_summary})

        file(APPEND "${summary_file}" "${__qt_configure_reports}")
    endif()

    # Show Qt specific notes, warnings, errors.
    if(__qt_configure_notes)
        message("${__qt_configure_notes}")
        file(APPEND "${summary_file}" "${__qt_configure_notes}")
    endif()
    if(__qt_configure_warnings)
        message("${__qt_configure_warnings}")
        file(APPEND "${summary_file}" "${__qt_configure_warnings}")
    endif()
    if(__qt_configure_errors)
        message("${__qt_configure_errors}")
        file(APPEND "${summary_file}" "${__qt_configure_errors}")
    endif()
    message("")
    if(__qt_configure_an_error_occurred)
        message(FATAL_ERROR "Check the configuration messages for an error that has occurred.")
    endif()
    file(APPEND "${summary_file}" "\n")
    set(QT_INTERNAL_SUMMARY_INSTRUCTIONS_SHOWN "TRUE" CACHE STRING "" FORCE)
endfunction()

# Takes a list of arguments, and saves them to be evaluated at the end of the configuration
# phase when the configuration summary is shown.
#
# RECORD_ON_FEATURE_EVALUATION option allows to record the command even while the feature
# evaluation-only stage.
function(qt_configure_record_command)
    cmake_parse_arguments(arg "RECORD_ON_FEATURE_EVALUATION"
                          ""
                          "" ${ARGV})
    # Don't record commands when only evaluating features of a configure.cmake file.
    if(__QtFeature_only_evaluate_features AND NOT arg_RECORD_ON_FEATURE_EVALUATION)
        return()
    endif()

    get_property(command_count GLOBAL PROPERTY qt_configure_command_count)

    if(NOT DEFINED command_count)
        set(command_count 0)
    endif()

    set_property(GLOBAL PROPERTY qt_configure_command_${command_count} "${arg_UNPARSED_ARGUMENTS}")

    math(EXPR command_count "${command_count}+1")
    set_property(GLOBAL PROPERTY qt_configure_command_count "${command_count}")
endfunction()

function(qt_configure_eval_commands)
    get_property(command_count GLOBAL PROPERTY qt_configure_command_count)
    if(NOT command_count)
        set(command_count 0)
    endif()
    set(command_index 0)

    while(command_index LESS command_count)
        get_property(command_args GLOBAL PROPERTY qt_configure_command_${command_index})
        if(NOT command_args)
            message(FATAL_ERROR
                "Empty arguments encountered while processing qt configure reports.")
        endif()

        list(POP_FRONT command_args command_name)
        if(command_name STREQUAL ADD_SUMMARY_SECTION)
            qt_configure_process_add_summary_section(${command_args})
        elseif(command_name STREQUAL END_SUMMARY_SECTION)
            qt_configure_process_end_summary_section(${command_args})
        elseif(command_name STREQUAL ADD_REPORT_ENTRY)
            qt_configure_process_add_report_entry(${command_args})
        elseif(command_name STREQUAL ADD_SUMMARY_ENTRY)
            qt_configure_process_add_summary_entry(${command_args})
        elseif(command_name STREQUAL ADD_BUILD_TYPE_AND_CONFIG)
            qt_configure_process_add_summary_build_type_and_config(${command_args})
        elseif(command_name STREQUAL ADD_BUILD_MODE)
            qt_configure_process_add_summary_build_mode(${command_args})
        elseif(command_name STREQUAL ADD_BUILD_PARTS)
            qt_configure_process_add_summary_build_parts(${command_args})
        endif()

        math(EXPR command_index "${command_index}+1")
    endwhile()

    # Propagate content to parent.
    set(__qt_configure_reports "${__qt_configure_reports}" PARENT_SCOPE)
    set(__qt_configure_notes "${__qt_configure_notes}" PARENT_SCOPE)
    set(__qt_configure_warnings "${__qt_configure_warnings}" PARENT_SCOPE)
    set(__qt_configure_errors "${__qt_configure_errors}" PARENT_SCOPE)
    set(__qt_configure_an_error_occurred "${__qt_configure_an_error_occurred}" PARENT_SCOPE)
endfunction()

macro(qt_configure_add_report message)
    string(APPEND __qt_configure_reports "\n${message}")
    set(__qt_configure_reports "${__qt_configure_reports}" PARENT_SCOPE)
endmacro()

macro(qt_configure_add_report_padded label message)
    qt_configure_get_padded_string("${__qt_configure_indent}${label}" "${message}" padded_message)
    string(APPEND __qt_configure_reports "\n${padded_message}")
    set(__qt_configure_reports "${__qt_configure_reports}" PARENT_SCOPE)
endmacro()

# Pad 'label' and 'value' with dots like this:
# "label ............... value"
#
# PADDING_LENGTH specifies the number of characters from the start to the last dot.
#                Default is 30.
# MIN_PADDING    specifies the minimum number of dots that are used for the padding.
#                Default is 0.
function(qt_configure_get_padded_string label value out_var)
    cmake_parse_arguments(arg "" "PADDING_LENGTH;MIN_PADDING" "" ${ARGN})
    if("${arg_MIN_PADDING}" STREQUAL "")
        set(arg_MIN_PADDING 0)
    endif()
    if(arg_PADDING_LENGTH)
        set(pad_string "")
        math(EXPR n "${arg_PADDING_LENGTH} - 1")
        foreach(i RANGE ${n})
            string(APPEND pad_string ".")
        endforeach()
    else()
        set(pad_string ".........................................")
    endif()
    string(LENGTH "${label}" label_len)
    string(LENGTH "${pad_string}" pad_len)
    math(EXPR pad_len "${pad_len}-${label_len}")
    if(pad_len LESS "0")
        set(pad_len ${arg_MIN_PADDING})
    endif()
    string(SUBSTRING "${pad_string}" 0 "${pad_len}" pad_string)
    set(output "${label} ${pad_string} ${value}")
    set("${out_var}" "${output}" PARENT_SCOPE)
endfunction()

function(qt_configure_add_summary_entry)
    qt_configure_record_command(ADD_SUMMARY_ENTRY ${ARGV})
endfunction()

function(qt_configure_process_add_summary_entry)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        ""
        "ARGS;TYPE;MESSAGE"
        "CONDITION")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_TYPE)
        set(arg_TYPE "feature")
    endif()

    if(NOT "${arg_CONDITION}" STREQUAL "")
        qt_evaluate_config_expression(condition_result ${arg_CONDITION})
        if(NOT condition_result)
            return()
        endif()
    endif()

    if(arg_TYPE STREQUAL "firstAvailableFeature")
        set(first_feature_found FALSE)
        set(message "")
        string(REPLACE " " ";" args_list "${arg_ARGS}")
        foreach(feature ${args_list})
            qt_feature_normalize_name("${feature}" feature)
            if(NOT DEFINED QT_FEATURE_${feature})
                message(FATAL_ERROR "Asking for a report on undefined feature ${feature}.")
            endif()
            if(QT_FEATURE_${feature})
                set(first_feature_found TRUE)
                set(message "${QT_FEATURE_LABEL_${feature}}")
                break()
            endif()
        endforeach()

        if(NOT first_feature_found)
            set(message "<none>")
        endif()
        qt_configure_add_report_padded("${arg_MESSAGE}" "${message}")
    elseif(arg_TYPE STREQUAL "featureList")
        set(available_features "")
        string(REPLACE " " ";" args_list "${arg_ARGS}")

        foreach(feature ${args_list})
            qt_feature_normalize_name("${feature}" feature)
            if(NOT DEFINED QT_FEATURE_${feature})
                message(FATAL_ERROR "Asking for a report on undefined feature ${feature}.")
            endif()
            if(QT_FEATURE_${feature})
                list(APPEND available_features "${QT_FEATURE_LABEL_${feature}}")
            endif()
        endforeach()

        if(NOT available_features)
            set(message "<none>")
        else()
            list(JOIN available_features " " message)
        endif()
        qt_configure_add_report_padded("${arg_MESSAGE}" "${message}")
    elseif(arg_TYPE STREQUAL "feature")
        qt_feature_normalize_name("${arg_ARGS}" feature)

        set(label "${QT_FEATURE_LABEL_${feature}}")

        if(NOT label)
            set(label "${feature}")
        endif()

        if(QT_FEATURE_${feature})
            set(value "yes")
        else()
            set(value "no")
        endif()

        qt_configure_add_report_padded("${label}" "${value}")
    elseif(arg_TYPE STREQUAL "message")
        qt_configure_add_report_padded("${arg_ARGS}" "${arg_MESSAGE}")
    endif()
endfunction()

function(qt_configure_add_summary_build_type_and_config)
    qt_configure_record_command(ADD_BUILD_TYPE_AND_CONFIG ${ARGV})
endfunction()

function(qt_configure_process_add_summary_build_type_and_config)
    get_property(subarch_summary GLOBAL PROPERTY qt_configure_subarch_summary)
    if(APPLE AND (CMAKE_OSX_ARCHITECTURES MATCHES ";"))
        set(message
            "Building for: ${QT_QMAKE_TARGET_MKSPEC} (${CMAKE_OSX_ARCHITECTURES}), ${TEST_architecture_arch} features: ${subarch_summary})")
    else()
        set(message
            "Building for: ${QT_QMAKE_TARGET_MKSPEC} (${TEST_architecture_arch}, CPU features: ${subarch_summary})")
    endif()
    qt_configure_add_report("${message}")

    set(message "Compiler: ")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        string(APPEND message "clang (Apple)")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
        string(APPEND message "clang (Intel LLVM)")
    elseif(CLANG)
        string(APPEND message "clang")
    elseif(QCC)
        string(APPEND message "rim_qcc")
    elseif(GCC)
        string(APPEND message "gcc")
    elseif(MSVC)
        string(APPEND message "msvc")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GHS")
        string(APPEND message "ghs")
    else()
        string(APPEND message "unknown (${CMAKE_CXX_COMPILER_ID})")
    endif()
    string(APPEND message " ${CMAKE_CXX_COMPILER_VERSION}")
    qt_configure_add_report("${message}")
endfunction()

function(qt_configure_add_summary_build_mode)
    qt_configure_record_command(ADD_BUILD_MODE ${ARGV})
endfunction()

function(qt_configure_process_add_summary_build_mode label)
    set(message "")
    if(DEFINED CMAKE_BUILD_TYPE)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            string(APPEND message "debug")
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            string(APPEND message "release")
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            string(APPEND message "release (with debug info)")
        else()
            string(APPEND message "${CMAKE_BUILD_TYPE}")
        endif()
    elseif(DEFINED CMAKE_CONFIGURATION_TYPES)
        if("Release" IN_LIST CMAKE_CONFIGURATION_TYPES
                AND "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
            string(APPEND message "debug and release")
        elseif("RelWithDebInfo" IN_LIST CMAKE_CONFIGURATION_TYPES
                AND "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
            string(APPEND message "debug and release (with debug info)")
        else()
            string(APPEND message "${CMAKE_CONFIGURATION_TYPES}")
        endif()
    endif()

    qt_configure_add_report_padded("${label}" "${message}")
endfunction()

function(qt_configure_add_summary_build_parts)
    qt_configure_record_command(ADD_BUILD_PARTS ${ARGV})
endfunction()

function(qt_configure_process_add_summary_build_parts label)
    qt_get_build_parts(parts)
    string(REPLACE ";" " " message "${parts}")
    qt_configure_add_report_padded("${label}" "${message}")
endfunction()

function(qt_configure_add_summary_section)
    qt_configure_record_command(ADD_SUMMARY_SECTION ${ARGV})
endfunction()

function(qt_configure_process_add_summary_section)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        ""
        "NAME"
        "")
    _qt_internal_validate_all_args_are_parsed(arg)

    qt_configure_add_report("${__qt_configure_indent}${arg_NAME}:")
    if(NOT DEFINED __qt_configure_indent)
        set(__qt_configure_indent "  " PARENT_SCOPE)
    else()
        set(__qt_configure_indent "${__qt_configure_indent}  " PARENT_SCOPE)
    endif()
endfunction()

function(qt_configure_end_summary_section)
    qt_configure_record_command(END_SUMMARY_SECTION ${ARGV})
endfunction()

function(qt_configure_process_end_summary_section)
    string(LENGTH "${__qt_configure_indent}" indent_len)
    if(indent_len GREATER_EQUAL 2)
        string(SUBSTRING "${__qt_configure_indent}" 2 -1 __qt_configure_indent)
        set(__qt_configure_indent "${__qt_configure_indent}" PARENT_SCOPE)
    endif()
endfunction()

function(qt_configure_add_report_entry)
    qt_configure_record_command(ADD_REPORT_ENTRY ${ARGV})
endfunction()

function(qt_configure_add_report_error error)
    message(SEND_ERROR "${error}")
    qt_configure_add_report_entry(TYPE ERROR MESSAGE "${error}" CONDITION TRUE ${ARGN})
endfunction()

# Goes through each token in given in `CONDITION` or `COMPILE_TESTS_TO_SHOW_ON_ERROR`, checks if
# the token starts with TEST_ which means it represents a Qt compile test, queries its
# compile output if available, and appends it to `out_var`.
# The compile output for a test is only available on first configuration, because we don't cache it
# across cmake invocations.
function(qt_internal_get_try_compile_output_from_tests_in_condition out_var)
    set(opt_args "")
    set(single_args "")
    set(multi_args
        CONDITION
        COMPILE_TESTS_TO_SHOW_ON_ERROR
    )
    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(content "")

    foreach(token IN LISTS arg_CONDITION arg_COMPILE_TESTS_TO_SHOW_ON_ERROR)
        if(token MATCHES "TEST_(.+)")
            set(name "${CMAKE_MATCH_1}")
            get_cmake_property(try_compile_output _qt_run_config_compile_test_output_${name})

            if(try_compile_output)
                string(APPEND content "\n   TEST_${name} output: \n\n${try_compile_output}")
            endif()
        else()
            continue()
        endif()
    endforeach()

    if(content)
        string(PREPEND content "\n Compile test outputs:\n")
    endif()

    set(${out_var} "${content}" PARENT_SCOPE)
endfunction()

function(qt_configure_process_add_report_entry)
    set(opt_args "")
    set(single_args
        TYPE
        MESSAGE
    )
    set(multi_args
        CONDITION
        COMPILE_TESTS_TO_SHOW_ON_ERROR
    )
    cmake_parse_arguments(PARSE_ARGV 0 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    set(possible_types NOTE WARNING ERROR FATAL_ERROR)
    if(NOT "${arg_TYPE}" IN_LIST possible_types)
        message(FATAL_ERROR "qt_configure_add_report_entry: '${arg_TYPE}' is not a valid type.")
    endif()

    if(NOT arg_MESSAGE)
        message(FATAL_ERROR "qt_configure_add_report_entry: Empty message given.")
    endif()

    if(arg_TYPE STREQUAL "NOTE")
        set(contents_var "__qt_configure_notes")
        set(prefix "Note: ")
    elseif(arg_TYPE STREQUAL "WARNING")
        set(contents_var "__qt_configure_warnings")
        set(prefix "WARNING: ")
    elseif(arg_TYPE STREQUAL "ERROR")
        set(contents_var "__qt_configure_errors")
        set(prefix "ERROR: ")
    elseif(arg_TYPE STREQUAL "FATAL_ERROR")
        set(contents_var "__qt_configure_errors")
        set(prefix "FATAL ERROR: ")
    endif()

    if(NOT "${arg_CONDITION}" STREQUAL "")
        qt_evaluate_config_expression(condition_result ${arg_CONDITION})
    endif()

    if("${arg_CONDITION}" STREQUAL "" OR condition_result)
        set(new_report "${prefix}${arg_MESSAGE}")

        set(compile_test_args "")
        if(arg_CONDITION)
            list(APPEND compile_test_args CONDITION ${arg_CONDITION})
        endif()
        if(arg_COMPILE_TESTS_TO_SHOW_ON_ERROR)
            list(APPEND compile_test_args
                COMPILE_TESTS_TO_SHOW_ON_ERROR ${arg_COMPILE_TESTS_TO_SHOW_ON_ERROR})
        endif()

        qt_internal_get_try_compile_output_from_tests_in_condition(extra_output
            ${compile_test_args}
        )
        if(extra_output)
            string(APPEND new_report "\n${extra_output}")
        endif()

        string(APPEND "${contents_var}" "\n${new_report}")

        if(arg_TYPE STREQUAL "ERROR" OR arg_TYPE STREQUAL "FATAL_ERROR")
            set(__qt_configure_an_error_occurred "TRUE" PARENT_SCOPE)
        endif()
    endif()

    set("${contents_var}" "${${contents_var}}" PARENT_SCOPE)
endfunction()
