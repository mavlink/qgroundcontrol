# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(PkgConfig QUIET)

pkg_check_modules(Libsystemd IMPORTED_TARGET "libsystemd")

if (NOT TARGET PkgConfig::Libsystemd)
    set(Libsystemd_FOUND 0)
endif()
