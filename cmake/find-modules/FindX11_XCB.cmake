# SPDX-FileCopyrightText: 2014 Alex Merry <alex.merry@kde.org>
# SPDX-FileCopyrightText: 2011 Fredrik Höglund <fredrik@kde.org>
# SPDX-FileCopyrightText: 2008 Helio Chissini de Castro <helio@kde.org>
# SPDX-FileCopyrightText: 2007 Matthias Kretz <kretz@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:
FindX11_XCB
-----------

Try to find the X11 XCB compatibility library.

This will define the following variables:

``X11_XCB_FOUND``
    True if (the requested version of) libX11-xcb is available
``X11_XCB_VERSION``
    The version of libX11-xcb (this is not guaranteed to be set even when
    X11_XCB_FOUND is true)
``X11_XCB_LIBRARIES``
    This can be passed to target_link_libraries() instead of the ``EGL::EGL``
    target
``X11_XCB_INCLUDE_DIR``
    This should be passed to target_include_directories() if the target is not
    used for linking
``X11_XCB_DEFINITIONS``
    This should be passed to target_compile_options() if the target is not
    used for linking

If ``X11_XCB_FOUND`` is TRUE, it will also define the following imported
target:

``X11::XCB``
    The X11 XCB compatibility library

In general we recommend using the imported target, as it is easier to use.
Bear in mind, however, that if the target is in the link interface of an
exported library, it must be made available by the package config file.

Since pre-1.0.0.
#]=======================================================================]

include(${CMAKE_CURRENT_LIST_DIR}/ECMFindModuleHelpersStub.cmake)

ecm_find_package_version_check(X11_XCB)

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
find_package(PkgConfig QUIET)
pkg_check_modules(PKG_X11_XCB QUIET x11-xcb)

set(X11_XCB_DEFINITIONS ${PKG_X11_XCB_CFLAGS_OTHER})
set(X11_XCB_VERSION ${PKG_X11_XCB_VERSION})

find_path(X11_XCB_INCLUDE_DIR
    NAMES X11/Xlib-xcb.h
    HINTS ${PKG_X11_XCB_INCLUDE_DIRS}
)
find_library(X11_XCB_LIBRARY
    NAMES X11-xcb
    HINTS ${PKG_X11_XCB_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(X11_XCB
    FOUND_VAR
        X11_XCB_FOUND
    REQUIRED_VARS
        X11_XCB_LIBRARY
        X11_XCB_INCLUDE_DIR
    VERSION_VAR
        X11_XCB_VERSION
)

if(X11_XCB_FOUND AND NOT TARGET X11::XCB)
    add_library(X11::XCB UNKNOWN IMPORTED)
    set_target_properties(X11::XCB PROPERTIES
        IMPORTED_LOCATION "${X11_XCB_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "${X11_XCB_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${X11_XCB_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(X11_XCB_INCLUDE_DIR X11_XCB_LIBRARY)

# compatibility variables
set(X11_XCB_LIBRARIES ${X11_XCB_LIBRARY})
set(X11_XCB_INCLUDE_DIRS ${X11_XCB_INCLUDE_DIR})
set(X11_XCB_VERSION_STRING ${X11_XCB_VERSION})

include(FeatureSummary)
set_package_properties(X11_XCB PROPERTIES
    URL "https://xorg.freedesktop.org/"
    DESCRIPTION "A compatibility library for code that translates Xlib API calls into XCB calls"
)
