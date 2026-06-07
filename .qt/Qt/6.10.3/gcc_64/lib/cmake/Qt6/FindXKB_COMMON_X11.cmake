# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(PkgConfig QUIET)

pkg_check_modules(XKB_COMMON_X11 IMPORTED_TARGET "xkbcommon-x11>=0.4.1")

if (NOT TARGET PkgConfig::XKB_COMMON_X11)
    set(XKB_COMMON_X11_FOUND 0)
endif()
