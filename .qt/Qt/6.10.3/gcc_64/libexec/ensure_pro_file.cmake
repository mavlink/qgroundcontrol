# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# CMake script to ensure that a qmake project file exists.
#
# Usage: cmake -DPRO_FILE=.../project.pro -P .../ensure_pro_file.cmake
#
# This script checks for existence of ${PRO_FILE} and creates a fake one, if needed.
#

if(NOT EXISTS "${PRO_FILE}")
    get_filename_component(dir "${PRO_FILE}" DIRECTORY)
    if(NOT IS_DIRECTORY "${dir}")
        file(MAKE_DIRECTORY "${dir}")
    endif()

    file(WRITE "${PRO_FILE}" "TEMPLATE = aux
")
endif()
