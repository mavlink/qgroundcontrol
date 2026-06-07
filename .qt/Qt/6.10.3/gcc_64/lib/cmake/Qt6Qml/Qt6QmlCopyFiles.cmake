# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

include("${CMAKE_CURRENT_LIST_DIR}/Qt6QmlPublicCMakeHelpers.cmake")

# Includes FILES_INFO_PATH, expects it to have the working directory, timestamp file and a list
# of file source and destination paths, to copy them from src to dest.
function(qt_internal_qml_copy_files)
    if(NOT FILES_INFO_PATH)
        message(FATAL_ERROR "FILES_INFO_PATH is not defined")
    endif()

    include("${FILES_INFO_PATH}")

    if(NOT working_dir)
        message(FATAL_ERROR "working_dir is not defined")
    endif()

    if(NOT timestamp_file)
        message(FATAL_ERROR "timestamp_file is not defined")
    endif()

    if(NOT src_and_dest_list)
        # Return early if there are no files to copy. We still need to touch the timestamp file to
        # ensure the script is not constantly reran.
        file(TOUCH "${timestamp_file}")
        return()
    endif()

    # Check there is an even number of paths in the input list.
    list(LENGTH src_and_dest_list src_and_dest_list_length)
    math(EXPR divide_by_two_remainder "${src_and_dest_list_length} % 2")
    if(divide_by_two_remainder STREQUAL "1")
        message(FATAL_ERROR "List of files to copy is corrupted, expected even number of paths.")
    endif()

    # Iterate every two list entries, the src and dest.
    set(step "2")
    math(EXPR final_idx "${src_and_dest_list_length} - 1" )
    foreach(idx RANGE 0 "${final_idx}" "${step}")
        # Extract the src path.
        list(GET src_and_dest_list "${idx}" src)

        # Extract the dest path.
        math(EXPR next_idx "${idx} + 1")
        list(GET src_and_dest_list "${next_idx}" dest)

        _qt_internal_qml_copy_file("${src}" "${dest}")
    endforeach()

    # Always touch the timestamp file to prevent reruns of the script.
    file(TOUCH "${timestamp_file}")
endfunction()

qt_internal_qml_copy_files()
