# SPDX-FileCopyrightText: 2006 Laurent Montel <montel@kde.org>
# SPDX-FileCopyrightText: 2019 Heiko Becker <heirecka@exherbo.org>
# SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:
FindTaglib
----------

Try to find the Taglib library.

This will define the following variables:

``Taglib_FOUND``
      True if the system has the taglib library of at least the minimum
      version specified by the version parameter to find_package()
``Taglib_INCLUDE_DIRS``
      The taglib include dirs for use with target_include_directories
``Taglib_LIBRARIES``
      The taglib libraries for use with target_link_libraries()
``Taglib_VERSION``
      The version of taglib that was found

If ``Taglib_FOUND`` is TRUE, it will also define the following imported
target:

``Taglib::Taglib``
      The Taglib library

Since 5.72.0
#]=======================================================================]

find_package(PkgConfig QUIET)

pkg_check_modules(PC_TAGLIB QUIET taglib)

find_path(Taglib_INCLUDE_DIRS
    NAMES tag.h
    PATH_SUFFIXES taglib
    HINTS ${PC_TAGLIB_INCLUDEDIR}
)

find_library(Taglib_LIBRARIES
    NAMES tag
    HINTS ${PC_TAGLIB_LIBDIR}
)

set(Taglib_VERSION ${PC_TAGLIB_VERSION})

if (Taglib_INCLUDE_DIRS AND NOT Taglib_VERSION)
    if(EXISTS "${Taglib_INCLUDE_DIRS}/taglib.h")
        file(READ "${Taglib_INCLUDE_DIRS}/taglib.h" TAGLIB_H)

        string(REGEX MATCH "#define TAGLIB_MAJOR_VERSION[ ]+[0-9]+" TAGLIB_MAJOR_VERSION_MATCH ${TAGLIB_H})
        string(REGEX MATCH "#define TAGLIB_MINOR_VERSION[ ]+[0-9]+" TAGLIB_MINOR_VERSION_MATCH ${TAGLIB_H})
        string(REGEX MATCH "#define TAGLIB_PATCH_VERSION[ ]+[0-9]+" TAGLIB_PATCH_VERSION_MATCH ${TAGLIB_H})

        string(REGEX REPLACE ".*_MAJOR_VERSION[ ]+(.*)" "\\1" TAGLIB_MAJOR_VERSION "${TAGLIB_MAJOR_VERSION_MATCH}")
        string(REGEX REPLACE ".*_MINOR_VERSION[ ]+(.*)" "\\1" TAGLIB_MINOR_VERSION "${TAGLIB_MINOR_VERSION_MATCH}")
        string(REGEX REPLACE ".*_PATCH_VERSION[ ]+(.*)" "\\1" TAGLIB_PATCH_VERSION "${TAGLIB_PATCH_VERSION_MATCH}")

        set(Taglib_VERSION "${TAGLIB_MAJOR_VERSION}.${TAGLIB_MINOR_VERSION}.${TAGLIB_PATCH_VERSION}")
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Taglib
    FOUND_VAR
        Taglib_FOUND
    REQUIRED_VARS
        Taglib_LIBRARIES
        Taglib_INCLUDE_DIRS
    VERSION_VAR
        Taglib_VERSION
)

if (Taglib_FOUND AND NOT TARGET Taglib::Taglib)
    add_library(Taglib::Taglib UNKNOWN IMPORTED)
    set_target_properties(Taglib::Taglib PROPERTIES
        IMPORTED_LOCATION "${Taglib_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${Taglib_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(Taglib_LIBRARIES Taglib_INCLUDE_DIRS)

include(FeatureSummary)
set_package_properties(Taglib PROPERTIES
    URL "https://taglib.org/"
    DESCRIPTION "A library for reading and editing the meta-data of audio formats"
)
