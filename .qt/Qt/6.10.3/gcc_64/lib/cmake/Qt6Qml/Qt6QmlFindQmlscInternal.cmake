# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (QT_NO_FIND_QMLSC)
    return()
endif()

set(QT_NO_FIND_QMLSC TRUE)

# Set up QT_HOST_PATH as an extra root path to look for the Tools package
# when cross-compiling.
if(NOT "${QT_HOST_PATH}" STREQUAL "" AND NOT QT_INTERNAL_SKIP_QMLSC_HOST_PATHS_ADJUSTMENT)
    set(_qt_backup_qmlsc_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    set(_qt_backup_qmlsc_CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH})
    set(_qt_backup_qmlsc_CMAKE_SYSROOT ${CMAKE_SYSROOT})

    list(PREPEND CMAKE_PREFIX_PATH "${QT_HOST_PATH_CMAKE_DIR}")
    if(_qt_additional_host_packages_prefix_paths)
        list(PREPEND CMAKE_PREFIX_PATH "${_qt_additional_host_packages_prefix_paths}")
    endif()

    list(PREPEND CMAKE_FIND_ROOT_PATH "${QT_HOST_PATH}")
    if(_qt_additional_host_packages_root_paths)
        list(PREPEND CMAKE_FIND_ROOT_PATH "${_qt_additional_host_packages_root_paths}")
    endif()
    if(NOT QT_INTERNAL_SKIP_QMLSC_SYSROOT_ADJUSTMENT)
        unset(CMAKE_SYSROOT)
    endif()
endif()

# This can't use the find_package(Qt6 COMPONENTS) signature, because Qt6Config uses NO_DEFAULT and
# won't look at the prepended extra find root paths.
get_target_property(_qt_qml_package_version Qt6::Qml _qt_package_version)
find_package(Qt6QmlCompilerPlusPrivateTools ${_qt_qml_package_version} QUIET CONFIG
    PATHS
            ${_qt_additional_host_packages_prefix_paths}
)

if(NOT "${QT_HOST_PATH}" STREQUAL "" AND NOT QT_INTERNAL_SKIP_QMLSC_HOST_PATHS_ADJUSTMENT)
    set(CMAKE_PREFIX_PATH ${_qt_backup_qmlsc_CMAKE_PREFIX_PATH})
    set(CMAKE_FIND_ROOT_PATH ${_qt_backup_qmlsc_CMAKE_FIND_ROOT_PATH})
    set(CMAKE_SYSROOT ${_qt_backup_qmlsc_CMAKE_SYSROOT})
    unset(_qt_backup_qmlsc_CMAKE_PREFIX_PATH)
    unset(_qt_backup_qmlsc_CMAKE_FIND_ROOT_PATH)
    unset(_qt_backup_qmlsc_CMAKE_SYSROOT)
endif()
