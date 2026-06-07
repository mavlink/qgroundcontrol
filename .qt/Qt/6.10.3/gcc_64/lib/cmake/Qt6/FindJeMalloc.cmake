# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(PkgConfig QUIET)

pkg_check_modules(JeMalloc IMPORTED_TARGET "jemalloc")

if (NOT TARGET PkgConfig::JeMalloc)
    set(JeMalloc_FOUND 0)
endif()
