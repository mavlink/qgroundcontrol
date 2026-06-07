# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Blake2 contains a reference implementation, libb2 is a more efficient
# implementation of a subset of Blake2 functions and should be preferred.
# This Find module only searches for libb2 for that reason.

if(TARGET Libb2::Libb2)
    set(Libb2_FOUND TRUE)
    return()
endif()

find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    pkg_check_modules(Libb2 IMPORTED_TARGET "libb2")

    if (TARGET PkgConfig::Libb2)
        add_library(Libb2::Libb2 INTERFACE IMPORTED)
        target_link_libraries(Libb2::Libb2 INTERFACE PkgConfig::Libb2)
        set(Libb2_FOUND TRUE)
    endif()
else()
    find_path(LIBB2_INCLUDE_DIR NAMES blake2.h)
    find_library(LIBB2_LIBRARY NAMES b2)

    if(LIBB2_LIBRARY AND LIBB2_INCLUDE_DIR)
        add_library(Libb2::Libb2 UNKNOWN IMPORTED)
        set_target_properties(Libb2::Libb2 PROPERTIES
            IMPORTED_LOCATION ${LIBB2_LIBRARY}
            INTERFACE_INCLUDE_DIRECTORIES ${LIBB2_INCLUDE_DIR}
        )
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Libb2 REQUIRED_VARS
        LIBB2_LIBRARY
        LIBB2_INCLUDE_DIR
    )
endif()
