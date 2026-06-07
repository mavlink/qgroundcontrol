# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This function wraps COMMAND with cmake script, that makes possible standalone run with external
# arguments.
#
# Generated wrapper will be written to OUTPUT_FILE.
# If WORKING_DIRECTORY is not set COMMAND will be executed in CMAKE_CURRENT_BINARY_DIR.
# Variables from ENVIRONMENT will be set before COMMAND execution.
# PRE_RUN and POST_RUN arguments may contain extra cmake code that supposed to be executed before
# and after COMMAND, respectively. Both arguments accept a list of cmake script language
# constructions. Each item of the list will be concantinated into single string with '\n' separator.
# COMMAND_ECHO option takes a value like it does for execute_process, and passes that value to
# execute_process.
function(_qt_internal_create_command_script)
    #This style of parsing keeps ';' in ENVIRONMENT variables
    cmake_parse_arguments(PARSE_ARGV 0 arg
                          ""
                          "OUTPUT_FILE;WORKING_DIRECTORY;COMMAND_ECHO"
                          "COMMAND;ENVIRONMENT;PRE_RUN;POST_RUN"
    )

    if(NOT arg_COMMAND)
        message(FATAL_ERROR "qt_internal_create_command_script: COMMAND is not specified")
    endif()

    if(NOT arg_OUTPUT_FILE)
        message(FATAL_ERROR "qt_internal_create_command_script: Wrapper OUTPUT_FILE\
is not specified")
    endif()

    if(NOT arg_WORKING_DIRECTORY AND NOT QNX)
        set(arg_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    set(environment_extras)
    set(skipNext false)
    if(arg_ENVIRONMENT)
        list(LENGTH arg_ENVIRONMENT length)
        math(EXPR length "${length} - 1")
        foreach(envIdx RANGE ${length})
            if(skipNext)
                set(skipNext FALSE)
                continue()
            endif()

            set(envVariable "")
            set(envValue "")

            list(GET arg_ENVIRONMENT ${envIdx} envVariable)
            math(EXPR envIdx "${envIdx} + 1")
            if (envIdx LESS_EQUAL ${length})
                list(GET arg_ENVIRONMENT ${envIdx} envValue)
            endif()

            if(NOT "${envVariable}" STREQUAL "")
                set(environment_extras "${environment_extras}\nset(ENV{${envVariable}} \
\"${envValue}\")")
            endif()
            set(skipNext TRUE)
        endforeach()
    endif()

    #Escaping environment variables before expand them by file GENERATE
    string(REPLACE "\\" "\\\\" environment_extras "${environment_extras}")

    if(CMAKE_HOST_WIN32)
        # It's necessary to call actual test inside 'cmd.exe', because 'execute_process' uses
        # SW_HIDE to avoid showing a console window, it affects other GUI as well.
        # See https://gitlab.kitware.com/cmake/cmake/-/issues/17690 for details.
        #
        # Run the command using the proxy 'call' command to avoid issues related to invalid
        # processing of quotes and spaces in cmd.exe arguments.
        set(extra_runner "cmd /c call")
    endif()

    if(arg_PRE_RUN)
        string(JOIN "\n" pre_run ${arg_PRE_RUN})
    endif()

    if(arg_POST_RUN)
        string(JOIN "\n" post_run ${arg_POST_RUN})
    endif()

    set(command_echo "")
    if(arg_COMMAND_ECHO)
        set(command_echo "COMMAND_ECHO ${arg_COMMAND_ECHO}")
    endif()

    set(command_escaped "")
    foreach(command_arg IN LISTS arg_COMMAND)
        if(NOT command_arg MATCHES "^\\\${[^}]+}$" AND NOT command_arg MATCHES "^\".+\"$")
            string(REPLACE "\"" "\\\"" command_arg "${command_arg}")
            string(APPEND command_escaped " \"${command_arg}\"")
        else()
            # Assume that all arguments that passed as escaped variables can be empty when
            # unwrapped during the script execution.
            string(APPEND command_escaped " ${command_arg}")
        endif()
    endforeach()

    file(GENERATE OUTPUT "${arg_OUTPUT_FILE}" CONTENT
"#!${CMAKE_COMMAND} -P
# Qt generated command wrapper

${environment_extras}
${pre_run}
execute_process(COMMAND ${extra_runner} ${command_escaped}
                WORKING_DIRECTORY \"${arg_WORKING_DIRECTORY}\"
                ${command_echo}
                RESULT_VARIABLE result
)
${post_run}
if(NOT result EQUAL 0)
    string(JOIN \" \" full_command ${extra_runner} ${command_escaped})
    message(FATAL_ERROR \"\${full_command} execution failed with exit code \${result}.\")
endif()"
    )
endfunction()

function(_qt_internal_test_batch_target_name out)
    set(${out} "test_batch" PARENT_SCOPE)
endfunction()

# Create a *_check target of the ctest execution for alternative execution
# Arguments:
# : CTEST_TEST_NAME: (default: ${testname})
#     name of the ctest test used
function(_qt_internal_make_check_target testname)
    set(options "")
    set(singleOpts CTEST_TEST_NAME)
    set(multiOpts "")

    cmake_parse_arguments(PARSE_ARGV 1 arg
            "${options}" "${singleOpts}" "${multiOpts}"
    )
    if(NOT arg_CTEST_TEST_NAME)
        set(arg_CTEST_TEST_NAME ${testname})
    endif()

    set(test_config_options "")
    get_cmake_property(is_multi_config GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(test_config_options -C $<CONFIG>)
    endif()
    # Note: By default the working directory here is CMAKE_CURRENT_BINARY_DIR, which will
    #   work as long as this is called anywhere up or down the path where the equivalent
    #   `add_test` is called (not down a different branch path).
    add_custom_target(${testname}_check
        VERBATIM
        COMMENT "Running ctest -V -R \"^${arg_CTEST_TEST_NAME}$\" ${test_config_options}"
        COMMAND
            "${CMAKE_CTEST_COMMAND}" -V -R "^${arg_CTEST_TEST_NAME}$" ${test_config_options}
    )
endfunction()
