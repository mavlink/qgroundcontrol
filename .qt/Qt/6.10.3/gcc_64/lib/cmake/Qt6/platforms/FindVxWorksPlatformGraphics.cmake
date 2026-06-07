# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#.rst:
# VxWorksPlatformGraphics
# ---------
find_package_handle_standard_args(VxWorksPlatformGraphics
    FOUND_VAR
        VxWorksPlatformGraphics_FOUND
    REQUIRED_VARS
        VxWorksPlatformGraphics_LIBRARIES_PACK
        VxWorksPlatformGraphics_REQUIRED_LIBRARIES
)

if(VxWorksPlatformGraphics_FOUND
        AND NOT TARGET VxWorksPlatformGraphics::VxWorksPlatformGraphics)
    add_library(VxWorksPlatformGraphics::VxWorksPlatformGraphics INTERFACE IMPORTED)
    set_target_properties(VxWorksPlatformGraphics::VxWorksPlatformGraphics PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${VxWorksPlatformGraphics_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${VxWorksPlatformGraphics_LIBRARIES_PACK}"
        INTERFACE_COMPILE_DEFINITIONS "${VxWorksPlatformGraphics_DEFINES}"
    )
    set(VxWorksPlatformGraphics_REQUIRED_DEFINITIONS ${VxWorksPlatformGraphics_DEFINES})
endif()

