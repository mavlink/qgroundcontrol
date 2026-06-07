# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Finish a preliminary .pc file.
#
# - Appends to each requirement the proper configuration postfix.
# - Strips empty PkgConfig fields.
#
# This file is to be used in CMake script mode with the following variables set:
# IN_FILE: path to the step 2 preliminary .pc file.
# OUT_FILE: path to the .pc file that is going to be created.
# POSTFIX: postfix for each requirement (optional).

cmake_policy(SET CMP0057 NEW)
file(STRINGS "${IN_FILE}" lines)
set(content "")
set(emptied "Libs:" "Cflags:" "Requires:")
foreach(line ${lines})
    if(POSTFIX)
        if(line MATCHES "^Libs:")
            string(REGEX REPLACE "( -lQt[^ ]+)" "\\1${POSTFIX}" line "${line}")
        elseif(line MATCHES "^Requires:")
            string(REGEX REPLACE "( Qt[^ ]+)" "\\1${POSTFIX}" line "${line}")
        endif()
    endif()
    string(REGEX REPLACE " +$" "" line "${line}")
    if(NOT line IN_LIST emptied)
        string(APPEND content "${line}\n")
    endif()
endforeach()
file(WRITE "${OUT_FILE}" "${content}")
