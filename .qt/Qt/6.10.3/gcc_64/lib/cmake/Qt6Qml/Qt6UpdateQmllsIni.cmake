# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

if(NOT EXISTS "${qmllsIniPath}")
    return()
endif()

file(READ "${qmllsIniPath}" qmllsIniContent)
string(REGEX REPLACE ".*buildDir=\"([^\"]*)\".*" "\\1" buildPaths "${qmllsIniContent}")

# replace unix path separator with CMake list separator
# note that the windows path separator is the CMake list separator already
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    string(REPLACE ":" ";" buildPaths "${buildPaths}")
endif()

foreach(path "${buildPaths}")
    file(GLOB_RECURSE qmllsIniFiles "${path}/*qmlls.ini.timestamp")
    if (qmllsIniFiles)
        file(REMOVE "${qmllsIniFiles}")
    endif()
endforeach()
