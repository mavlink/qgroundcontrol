# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

macro(qt_internal_setup_cmake_and_export_namespace)
    # The variables might have already been set in QtBuildInternalsExtra.cmake if the file is
    # included while building a new module and not QtBase. In that case, stop overriding the value.
    if(NOT INSTALL_CMAKE_NAMESPACE)
        set(INSTALL_CMAKE_NAMESPACE "Qt${PROJECT_VERSION_MAJOR}"
            CACHE STRING "CMake namespace [Qt${PROJECT_VERSION_MAJOR}]")
    endif()
    if(NOT QT_CMAKE_EXPORT_NAMESPACE)
        set(QT_CMAKE_EXPORT_NAMESPACE "Qt${PROJECT_VERSION_MAJOR}"
            CACHE STRING "CMake namespace used when exporting targets [Qt${PROJECT_VERSION_MAJOR}]")
    endif()
endmacro()
