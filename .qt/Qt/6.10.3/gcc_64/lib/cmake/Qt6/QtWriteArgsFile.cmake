# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# This script writes its arguments to the file determined by OUT_FILE.
# Each argument appears on a separate line.
# This is used for writing the config.opt file.
#
# This script takes the following arguments:
# IN_FILE: The input file. The whole command line as one string, or one argument per line.
# REDO_FILE: A file containing extra commands to be joined with IN_FILE.
# OUT_FILE: The output file. One argument per line.
# SKIP_ARGS: Number of arguments to skip from the front of the arguments list.
# SKIP_REDO_FILE_ARGS: Number of arguments to skip from the front of the redo file arguments list.
# IGNORE_ARGS: List of arguments to be ignored, i.e. that are not written.
#
# If the REDO_FILE is given, its parameters will be merged with IN_FILE parameters
# and be written into the OUT_FILE.

cmake_minimum_required(VERSION 3.16)

# Read arguments from IN_FILE and separate them.
file(READ "${IN_FILE}" raw_args)
# To catch cases where the path ends with an `\`, e.g., `-prefix "C:\Path\"`
string(REPLACE "\\\"" "\"" raw_args "${raw_args}")
string(REPLACE ";" "[[;]]" raw_args "${raw_args}")

separate_arguments(args NATIVE_COMMAND "${raw_args}")

string(REPLACE "\;" ";" args "${args}")
string(REPLACE "[[;]]" "\;" args "${args}")

if(DEFINED REDO_FILE)
    file(READ "${REDO_FILE}" raw_redo_args)
    separate_arguments(redo_args NATIVE_COMMAND "${raw_redo_args}")
    if(DEFINED SKIP_REDO_FILE_ARGS)
        foreach(i RANGE 1 ${SKIP_REDO_FILE_ARGS})
            list(POP_FRONT redo_args)
        endforeach()
    endif()

    if(args)
        list(FIND args "--" args_ddash_loc)
        list(FIND redo_args "--" redo_ddash_loc)
        if("${redo_ddash_loc}" STREQUAL "-1")
            if("${args_ddash_loc}" STREQUAL "-1")
                list(LENGTH args args_ddash_loc)
            endif()
            # Avoid adding an empty line for an empty -redo
            if(NOT "${redo_args}" STREQUAL "")
                list(INSERT args ${args_ddash_loc} "${redo_args}")
            endif()
        else()
            # Handling redo's configure options
            list(SUBLIST redo_args 0 ${redo_ddash_loc} redo_config_args)
            if(redo_config_args)
                if("${args_ddash_loc}" STREQUAL "-1")
                    list(APPEND args "${redo_config_args}")
                else()
                    list(INSERT args ${args_ddash_loc} "${redo_config_args}")
                endif()
            endif()

            # Handling redo's CMake options
            list(LENGTH redo_args redo_args_len)
            math(EXPR redo_ddash_loc "${redo_ddash_loc} + 1")
            # Catch an unlikely case of -redo being called with an empty --, ie., `-redo --`
            if(NOT ${redo_ddash_loc} STREQUAL ${redo_args_len})
                list(SUBLIST redo_args ${redo_ddash_loc} -1 redo_cmake_args)
            endif()

            if(DEFINED redo_cmake_args)
                if("${args_ddash_loc}" STREQUAL "-1")
                    list(APPEND args "--")
                endif()
                list(APPEND args "${redo_cmake_args}")
            endif()
        endif()
    else()
        list(APPEND args "${redo_args}")
    endif()
endif()

# Skip arguments if requested
if(DEFINED SKIP_ARGS)
    foreach(i RANGE 1 ${SKIP_ARGS})
        list(POP_FRONT args)
    endforeach()
endif()

# Write config.opt
set(content "")
foreach(arg IN LISTS args)
    if(NOT arg IN_LIST IGNORE_ARGS)
        string(APPEND content "${arg}\n")
    endif()
endforeach()

file(WRITE "${OUT_FILE}" "${content}")
