# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# FindGObject
# ---------
#
# Try to locate the gobject-2.0 library.
# If found, this will define the following variables:
#
# ``GObject_FOUND``
#     True if the gobject-2.0 library is available
#
# If ``GObject_FOUND`` is TRUE, it will also define the following
# imported target:
#
# ``GObject::GObject``
#     The gobject-2.0 library

include(CMakeFindDependencyMacro)
find_dependency(GLIB2)
qt_internal_disable_find_package_global_promotion(GLIB2::GLIB2)

if(NOT TARGET GObject::GObject)
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_GOBJECT gobject-2.0 IMPORTED_TARGET)
    if(TARGET PkgConfig::PC_GOBJECT)
        add_library(GObject::GObject INTERFACE IMPORTED)
        target_link_libraries(GObject::GObject INTERFACE
                            PkgConfig::PC_GOBJECT
                            GLIB2::GLIB2
        )
    else()
        find_path(GObject_INCLUDE_DIR
            NAMES gobject.h
            PATH_SUFFIXES glib-2.0/gobject/
        )
        find_library(GObject_LIBRARY NAMES gobject-2.0)
        if(GObject_LIBRARY AND GObject_INCLUDE_DIR)
            add_library(GObject::GObject INTERFACE IMPORTED)
            target_include_directories(GObject::GObject INTERFACE
                                    ${GObject_INCLUDE_DIR}
            )
            target_link_libraries(GObject::GObject INTERFACE
                                ${GObject_LIBRARY}
                                GLIB2::GLIB2
            )
        endif()
        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(GObject REQUIRED_VARS
                                        GObject_LIBRARY
                                        GObject_INCLUDE_DIR
    )
    endif()
endif()

if(TARGET GObject::GObject)
    set(GObject_FOUND TRUE)
endif()
