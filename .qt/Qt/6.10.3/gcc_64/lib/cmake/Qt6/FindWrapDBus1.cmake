# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# DBus1 is buggy and breaks PKG_CONFIG environment.
# Work around that:-/
# See https://gitlab.freedesktop.org/dbus/dbus/issues/267 for more information

# When doing top-level static Qt builds, we need to protect against double creation of the dbus
# target.
if(DBus1_FOUND OR WrapDBus1_FOUND OR TARGET dbus-1)
    set(WrapDBus1_FOUND 1)
    return()
endif()

if(DEFINED ENV{PKG_CONFIG_DIR})
    set(__qt_dbus_pcd "$ENV{PKG_CONFIG_DIR}")
endif()
if(DEFINED ENV{PKG_CONFIG_PATH})
    set(__qt_dbus_pcp "$ENV{PKG_CONFIG_PATH}")
endif()
if(DEFINED ENV{PKG_CONFIG_LIBDIR})
    set(__qt_dbus_pcl "$ENV{PKG_CONFIG_LIBDIR}")
endif()

find_package(DBus1 ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET)

if(DEFINED __qt_dbus_pcd)
    set(ENV{PKG_CONFIG_DIR} "${__qt_dbus_pcd}")
else()
    unset(ENV{PKG_CONFIG_DIR})
endif()
if(DEFINED __qt_dbus_pcp)
    set(ENV{PKG_CONFIG_PATH} "${__qt_dbus_pcp}")
else()
    unset(ENV{PKG_CONFIG_PATH})
endif()
if(DEFINED __qt_dbus_pcl)
    set(ENV{PKG_CONFIG_LIBDIR} "${__qt_dbus_pcl}")
else()
    unset(ENV{PKG_CONFIG_LIBDIR})
endif()

if(DBus1_FOUND)
    set(WrapDBus1_FOUND 1)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapDBus1 REQUIRED_VARS
                                            DBus1_LIBRARY DBus1_INCLUDE_DIR WrapDBus1_FOUND
                                            VERSION_VAR DBus1_VERSION)
