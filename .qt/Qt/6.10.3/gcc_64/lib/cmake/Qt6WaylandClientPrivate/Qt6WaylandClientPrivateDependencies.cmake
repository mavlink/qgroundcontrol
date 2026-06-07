# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Make sure Qt6 is found before anything else.
set(Qt6WaylandClientPrivate_FOUND FALSE)

if("${_qt_cmake_dir}" STREQUAL "")
    set(_qt_cmake_dir "${QT_TOOLCHAIN_RELOCATABLE_CMAKE_DIR}")
endif()
set(__qt_use_no_default_path_for_qt_packages "NO_DEFAULT_PATH")
if(QT_DISABLE_NO_DEFAULT_PATH_IN_QT_PACKAGES)
    set(__qt_use_no_default_path_for_qt_packages "")
endif()

# Don't propagate REQUIRED so we don't immediately FATAL_ERROR, rather let the find_dependency calls
# set _NOT_FOUND_MESSAGE which will be displayed by the includer of the Dependencies file.
set(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED FALSE)

if(NOT Qt6_FOUND)
    find_dependency(Qt6 6.10.3
        PATHS
            ${QT_BUILD_CMAKE_PREFIX_PATH}
            "${CMAKE_CURRENT_LIST_DIR}/.."
            "${_qt_cmake_dir}"
            ${_qt_additional_packages_prefix_paths}
        ${__qt_use_no_default_path_for_qt_packages}
    )
endif()


# note: _third_party_deps example: "ICU\\;FALSE\\;1.0\\;i18n uc data;ZLIB\\;FALSE\\;\\;"
set(__qt_WaylandClientPrivate_third_party_deps "")

if(__qt_WaylandClientPrivate_third_party_deps)
    _qt_internal_find_third_party_dependencies("WaylandClientPrivate" __qt_WaylandClientPrivate_third_party_deps)
endif()
unset(__qt_WaylandClientPrivate_third_party_deps)

# Find Qt tool package.
set(__qt_WaylandClientPrivate_tool_deps "")
if(__qt_WaylandClientPrivate_tool_deps)
    _qt_internal_find_tool_dependencies("WaylandClientPrivate" __qt_WaylandClientPrivate_tool_deps)
endif()
unset(__qt_WaylandClientPrivate_tool_deps)

# note: target_deps example: "Qt6Core\;5.12.0;Qt6Gui\;5.12.0"
set(__qt_WaylandClientPrivate_target_deps "Qt6WaylandClient\;6.10.3;Qt6CorePrivate\;6.10.3;Qt6GuiPrivate\;6.10.3;Qt6WaylandGlobalPrivate\;6.10.3")
set(__qt_WaylandClientPrivate_find_dependency_paths "${CMAKE_CURRENT_LIST_DIR}/.." "${_qt_cmake_dir}")
if(__qt_WaylandClientPrivate_target_deps)
    _qt_internal_find_qt_dependencies("WaylandClientPrivate" __qt_WaylandClientPrivate_target_deps
                                      __qt_WaylandClientPrivate_find_dependency_paths)
endif()
unset(__qt_WaylandClientPrivate_target_deps)
unset(__qt_WaylandClientPrivate_find_dependency_paths)

set(_Qt6WaylandClientPrivate_MODULE_DEPENDENCIES "WaylandClient;CorePrivate;GuiPrivate;WaylandGlobalPrivate")
set(Qt6WaylandClientPrivate_FOUND TRUE)
