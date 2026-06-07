# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Find the PPS library

# Will make the target PPS::PPS available when found.
if(TARGET PPS::PPS)
    set(PPS_FOUND TRUE)
    return()
endif()

find_library(PPS_LIBRARY NAMES "pps")
find_path(PPS_INCLUDE_DIR NAMES "sys/pps.h" DOC "The PPS Include path")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PPS DEFAULT_MSG PPS_INCLUDE_DIR PPS_LIBRARY)

mark_as_advanced(PPS_INCLUDE_DIR PPS_LIBRARY)

if(PPS_FOUND)
    add_library(PPS::PPS INTERFACE IMPORTED)
    target_link_libraries(PPS::PPS INTERFACE "${PPS_LIBRARY}")
    target_include_directories(PPS::PPS INTERFACE "${PPS_INCLUDE_DIR}")
endif()
