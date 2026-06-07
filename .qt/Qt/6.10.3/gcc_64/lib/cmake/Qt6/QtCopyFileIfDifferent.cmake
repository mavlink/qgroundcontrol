# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# copy_if_different works incorrect in Windows if file size if bigger than 2GB.
# See https://gitlab.kitware.com/cmake/cmake/-/issues/23052 and QTBUG-99491 for details.

cmake_minimum_required(VERSION 3.16)

set(copy_strategy "copy_if_different")
if(CMAKE_HOST_WIN32)
    file(SIZE "${SRC_FILE_PATH}" size)
    # If file size is bigger than 2GB copy it unconditionally
    if(size GREATER_EQUAL 2147483648)
        set(copy_strategy "copy")
    endif()
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E ${copy_strategy} "${SRC_FILE_PATH}" "${DST_FILE_PATH}")
