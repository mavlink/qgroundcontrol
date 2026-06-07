# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (TARGET PkgConfig::Waylandkms)
    set(Waylandkms_FOUND 1)
    return()
endif()

find_package(PkgConfig QUIET)

pkg_check_modules(Waylandkms wayland-kms IMPORTED_TARGET)

if (NOT TARGET PkgConfig::Waylandkms)
    set(Waylandkms_FOUND 0)
endif()
