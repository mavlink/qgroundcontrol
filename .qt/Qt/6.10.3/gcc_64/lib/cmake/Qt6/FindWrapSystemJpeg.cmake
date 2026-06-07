# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET WrapSystemJpeg::WrapSystemJpeg)
    set(WrapSystemJpeg_FOUND TRUE)
    return()
endif()
set(WrapSystemJpeg_REQUIRED_VARS __jpeg_found)

find_package(JPEG ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET)

set(__jpeg_target_name "JPEG::JPEG")
if(JPEG_FOUND AND TARGET "${__jpeg_target_name}")
    set(__jpeg_found TRUE)
endif()

if(JPEG_LIBRARIES)
    list(PREPEND WrapSystemJpeg_REQUIRED_VARS JPEG_LIBRARIES)
endif()
if(JPEG_VERSION)
    set(WrapSystemJpeg_VERSION "${JPEG_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemJpeg
                                  REQUIRED_VARS ${WrapSystemJpeg_REQUIRED_VARS}
                                  VERSION_VAR WrapSystemJpeg_VERSION)

if(WrapSystemJpeg_FOUND)
    add_library(WrapSystemJpeg::WrapSystemJpeg INTERFACE IMPORTED)
    target_link_libraries(WrapSystemJpeg::WrapSystemJpeg
                          INTERFACE "${__jpeg_target_name}")
endif()
unset(__jpeg_target_name)
unset(__jpeg_found)
