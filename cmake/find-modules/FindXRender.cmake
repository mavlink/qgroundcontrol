# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(PkgConfig QUIET)

if(NOT TARGET PkgConfig::XRender)
    pkg_check_modules(XRender IMPORTED_TARGET "xrender")

    if (NOT TARGET PkgConfig::XRender)
        set(XRender_FOUND 0)
    endif()
else()
    set(XRender_FOUND 1)
endif()
