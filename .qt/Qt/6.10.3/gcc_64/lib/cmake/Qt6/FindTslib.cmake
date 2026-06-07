# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(PkgConfig QUIET)

pkg_check_modules(Tslib IMPORTED_TARGET "tslib")

if (NOT TARGET PkgConfig::Tslib)
    set(Tslib_FOUND 0)
endif()
