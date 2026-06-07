# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# We can't create the same interface imported target multiple times, CMake will complain if we do
# that. This can happen if the find_package call is done in multiple different subdirectories.
if(TARGET WrapSystemZLIB::WrapSystemZLIB)
    set(WrapSystemZLIB_FOUND ON)
    return()
endif()

set(WrapSystemZLIB_FOUND OFF)

find_package(ZLIB ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION})

if(ZLIB_FOUND)
    set(WrapSystemZLIB_FOUND ON)

    add_library(WrapSystemZLIB::WrapSystemZLIB INTERFACE IMPORTED)
    if(APPLE)
        # On Darwin platforms FindZLIB sets IMPORTED_LOCATION to the absolute path of the library
        # within the framework. This ends up as an absolute path link flag, which we don't want,
        # because that makes our .prl files un-relocatable and also breaks iOS simulator_and_device
        # SDK switching in Xcode.
        # Just pass a linker flag instead.
        target_link_libraries(WrapSystemZLIB::WrapSystemZLIB INTERFACE "-lz")
    else()
        target_link_libraries(WrapSystemZLIB::WrapSystemZLIB INTERFACE ZLIB::ZLIB)
    endif()
endif()

if(ZLIB_VERSION)
    set(WrapSystemZLIB_VERSION "${ZLIB_VERSION}")
elseif(ZLIB_VERSION_STRING)
    set(WrapSystemZLIB_VERSION "${ZLIB_VERSION_STRING}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemZLIB
                                  REQUIRED_VARS WrapSystemZLIB_FOUND
                                  VERSION_VAR WrapSystemZLIB_VERSION)
