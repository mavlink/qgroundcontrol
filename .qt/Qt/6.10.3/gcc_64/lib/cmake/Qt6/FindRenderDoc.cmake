# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(WIN32 OR UNIX)
    find_path(RenderDoc_INCLUDE_DIR
        NAMES renderdoc_app.h)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RenderDoc
    DEFAULT_MSG
    RenderDoc_INCLUDE_DIR)

mark_as_advanced(RenderDoc_INCLUDE_DIR)

if(RenderDoc_FOUND AND NOT TARGET RenderDoc::RenderDoc)
    add_library(RenderDoc::RenderDoc INTERFACE IMPORTED)
    set_target_properties(RenderDoc::RenderDoc PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${RenderDoc_INCLUDE_DIR}")
endif()
