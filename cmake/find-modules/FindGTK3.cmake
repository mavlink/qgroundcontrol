# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

find_package(PkgConfig QUIET)

set(__gtk3_required_version "${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION}")
if(__gtk3_required_version)
    set(__gtk3_required_version " >= ${__gtk3_required_version}")
endif()
pkg_check_modules(GTK3 IMPORTED_TARGET "gtk+-3.0${__gtk3_required_version}")

if (NOT TARGET PkgConfig::GTK3)
    set(GTK3_FOUND 0)
endif()
unset(__gtk3_required_version)
