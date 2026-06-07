# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Make sure Qt6 is found before anything else.
set(Qt6XcbQpaPrivate_FOUND FALSE)

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
set(__qt_XcbQpaPrivate_third_party_deps "XKB_COMMON_X11\;FALSE\;0.9.0\;\;;XCB\;FALSE\;0.1.1\;CURSOR\;;XCB\;FALSE\;0.3.9\;ICCCM\;;XCB\;FALSE\;0.3.8\;UTIL\;;XCB\;FALSE\;0.3.9\;IMAGE\;;XCB\;FALSE\;0.3.9\;KEYSYMS\;;XCB\;FALSE\;\;RANDR\;;XCB\;FALSE\;\;RENDER\;;XCB\;FALSE\;0.3.9\;RENDERUTIL\;;XCB\;FALSE\;\;SHAPE\;;XCB\;FALSE\;\;SHM\;;XCB\;FALSE\;\;SYNC\;;XCB\;FALSE\;1.11\;\;;XCB\;FALSE\;\;XFIXES\;;XCB\;FALSE\;\;XKB\;;XKB\;FALSE\;0.9.0\;\;;X11_XCB\;FALSE\;\;\;;X11\;FALSE\;\;\;")
set(__qt_XcbQpaPrivate_third_party_package_XKB_COMMON_X11_provided_targets "PkgConfig::XKB_COMMON_X11")
set(__qt_XcbQpaPrivate_third_party_package_XCB-CURSOR_provided_targets "XCB::CURSOR")
set(__qt_XcbQpaPrivate_third_party_package_XCB-ICCCM_provided_targets "XCB::ICCCM")
set(__qt_XcbQpaPrivate_third_party_package_XCB-UTIL_provided_targets "XCB::UTIL")
set(__qt_XcbQpaPrivate_third_party_package_XCB-IMAGE_provided_targets "XCB::IMAGE")
set(__qt_XcbQpaPrivate_third_party_package_XCB-KEYSYMS_provided_targets "XCB::KEYSYMS")
set(__qt_XcbQpaPrivate_third_party_package_XCB-RANDR_provided_targets "XCB::RANDR")
set(__qt_XcbQpaPrivate_third_party_package_XCB-RENDER_provided_targets "XCB::RENDER")
set(__qt_XcbQpaPrivate_third_party_package_XCB-RENDERUTIL_provided_targets "XCB::RENDERUTIL")
set(__qt_XcbQpaPrivate_third_party_package_XCB-SHAPE_provided_targets "XCB::SHAPE")
set(__qt_XcbQpaPrivate_third_party_package_XCB-SHM_provided_targets "XCB::SHM")
set(__qt_XcbQpaPrivate_third_party_package_XCB-SYNC_provided_targets "XCB::SYNC")
set(__qt_XcbQpaPrivate_third_party_package_XCB_provided_targets "XCB::XCB")
set(__qt_XcbQpaPrivate_third_party_package_XCB-XFIXES_provided_targets "XCB::XFIXES")
set(__qt_XcbQpaPrivate_third_party_package_XCB-XKB_provided_targets "XCB::XKB")
set(__qt_XcbQpaPrivate_third_party_package_XKB_provided_targets "XKB::XKB")
set(__qt_XcbQpaPrivate_third_party_package_X11_XCB_provided_targets "X11::XCB")
set(__qt_XcbQpaPrivate_third_party_package_X11_provided_targets "X11::X11")

if(__qt_XcbQpaPrivate_third_party_deps)
    _qt_internal_find_third_party_dependencies("XcbQpaPrivate" __qt_XcbQpaPrivate_third_party_deps)
endif()
unset(__qt_XcbQpaPrivate_third_party_deps)

# Find Qt tool package.
set(__qt_XcbQpaPrivate_tool_deps "")
if(__qt_XcbQpaPrivate_tool_deps)
    _qt_internal_find_tool_dependencies("XcbQpaPrivate" __qt_XcbQpaPrivate_tool_deps)
endif()
unset(__qt_XcbQpaPrivate_tool_deps)

# note: target_deps example: "Qt6Core\;5.12.0;Qt6Gui\;5.12.0"
set(__qt_XcbQpaPrivate_target_deps "Qt6CorePrivate\;6.10.3;Qt6GuiPrivate\;6.10.3")
set(__qt_XcbQpaPrivate_find_dependency_paths "${CMAKE_CURRENT_LIST_DIR}/.." "${_qt_cmake_dir}")
if(__qt_XcbQpaPrivate_target_deps)
    _qt_internal_find_qt_dependencies("XcbQpaPrivate" __qt_XcbQpaPrivate_target_deps
                                      __qt_XcbQpaPrivate_find_dependency_paths)
endif()
unset(__qt_XcbQpaPrivate_target_deps)
unset(__qt_XcbQpaPrivate_find_dependency_paths)

set(_Qt6XcbQpaPrivate_MODULE_DEPENDENCIES "CorePrivate;GuiPrivate")
set(Qt6XcbQpaPrivate_FOUND TRUE)
