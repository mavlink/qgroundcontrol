# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# FindMMRenderer
# ---------
#
# Try to locate the mm-renderer library.
# If found, this will define the following variables:
#
# ``MMRenderer_FOUND``
#     True if the mm-renderer library is available
# ``MMRenderer_LIBRARY``
#     The mm-renderer library
#
# If ``MMRenderer_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``MMRenderer::MMRenderer``
#     The mm-renderer library to link to

find_library(MMRenderer_LIBRARY NAMES mmrndclient)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MMRenderer DEFAULT_MSG MMRenderer_LIBRARY)
if(MMRenderer_FOUND AND NOT TARGET MMRenderer::MMRenderer)
    add_library(MMRenderer::MMRenderer INTERFACE IMPORTED)
    target_link_libraries(MMRenderer::MMRenderer
                        INTERFACE "${MMRenderer_LIBRARY}")
endif()
mark_as_advanced(MMRenderer_LIBRARY)
