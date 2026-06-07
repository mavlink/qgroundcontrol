# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#.rst:
# IntegrityPlatformGraphics
# ---------

# Temporary fall back to allow integrating this pachset without the
# toolchain file updated.
if(NOT IntegrityPlatformGraphics_REQUIRED_LIBRARIES)
    set(IntegrityPlatformGraphics_REQUIRED_LIBRARIES ${_qt_igy_gui_libs})
endif()

find_package_handle_standard_args(IntegrityPlatformGraphics
    FOUND_VAR
        IntegrityPlatformGraphics_FOUND
    REQUIRED_VARS
        IntegrityPlatformGraphics_LIBRARY
        IntegrityPlatformGraphics_INCLUDE_DIR
        IntegrityPlatformGraphics_REQUIRED_LIBRARIES
)

if(IntegrityPlatformGraphics_FOUND
        AND NOT TARGET IntegrityPlatformGraphics::IntegrityPlatformGraphics)
    add_library(IntegrityPlatformGraphics::IntegrityPlatformGraphics STATIC IMPORTED)
    set_target_properties(IntegrityPlatformGraphics::IntegrityPlatformGraphics PROPERTIES
        IMPORTED_LOCATION "${IntegrityPlatformGraphics_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${IntegrityPlatformGraphics_INCLUDE_DIR}"
    )
    target_link_libraries(IntegrityPlatformGraphics::IntegrityPlatformGraphics
        INTERFACE ${IntegrityPlatformGraphics_LIBRARIES_PACK})
endif()

mark_as_advanced(IntegrityPlatformGraphics_LIBRARY)

# compatibility variables
set(IntegrityPlatformGraphics_LIBRARIES ${IntegrityPlatformGraphics_LIBRARY})
