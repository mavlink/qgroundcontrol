# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(QmlAssetDownloaderplugin_FOUND TRUE)

# note: _third_party_deps example: "ICU\\;FALSE\\;1.0\\;i18n uc data;ZLIB\\;FALSE\\;\\;"
set(__qt_QmlAssetDownloaderplugin_third_party_deps "")
if(__qt_QmlAssetDownloaderplugin_third_party_deps)
    _qt_internal_find_third_party_dependencies("QmlAssetDownloaderplugin" __qt_QmlAssetDownloaderplugin_third_party_deps)
endif()
unset(__qt_QmlAssetDownloaderplugin_third_party_deps)

set(__qt_use_no_default_path_for_qt_packages "NO_DEFAULT_PATH")
if(QT_DISABLE_NO_DEFAULT_PATH_IN_QT_PACKAGES)
    set(__qt_use_no_default_path_for_qt_packages "")
endif()

# note: target_deps example: "Qt6Core\;5.12.0;Qt6Gui\;5.12.0"
set(__qt_QmlAssetDownloaderplugin_target_deps "Qt6QmlAssetDownloader\;6.10.3")
set(__qt_QmlAssetDownloaderplugin_find_dependency_paths "${CMAKE_CURRENT_LIST_DIR}/../..;${_qt_cmake_dir}")
if(__qt_QmlAssetDownloaderplugin_target_deps)
    _qt_internal_find_qt_dependencies("QmlAssetDownloaderplugin" __qt_QmlAssetDownloaderplugin_target_deps
                                      __qt_QmlAssetDownloaderplugin_find_dependency_paths)
endif()
unset(__qt_QmlAssetDownloaderplugin_target_deps)
unset(__qt_QmlAssetDownloaderplugin_find_dependency_paths)

if(__qt_${target}_missing_deps)
    set(QmlAssetDownloaderplugin_FOUND FALSE)
endif()
