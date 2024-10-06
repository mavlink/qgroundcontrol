# Try to find xkbcommon on a Unix system
#
# This will define:
#
#   XKB_FOUND        - True if XKB is available
#   XKB_LIBRARIES    - Link these to use XKB
#   XKB_INCLUDE_DIRS - Include directory for XKB
#   XKB_DEFINITIONS  - Compiler flags for using XKB
#
# Additionally, the following imported targets will be defined:
#
#   XKB::XKB
#
# Copyright (c) 2014 Martin Gräßlin <mgraesslin@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. Neither the name of the University nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

if(CMAKE_VERSION VERSION_LESS 2.8.12)
    message(FATAL_ERROR "CMake 2.8.12 is required by FindXKB.cmake")
endif()
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
    message(AUTHOR_WARNING "Your project should require at least CMake 2.8.12 to use FindXKB.cmake")
endif()

if(NOT WIN32)
    # Use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig QUIET)
    pkg_check_modules(PKG_XKB QUIET xkbcommon)

    set(XKB_DEFINITIONS ${PKG_XKB_CFLAGS_OTHER})

    find_path(XKB_INCLUDE_DIR
        NAMES
            xkbcommon/xkbcommon.h
        HINTS
            ${PKG_XKB_INCLUDE_DIRS}
    )
    find_library(XKB_LIBRARY
        NAMES
            xkbcommon
        HINTS
            ${PKG_XKB_LIBRARY_DIRS}
    )

    set(XKB_LIBRARIES ${XKB_LIBRARY})
    set(XKB_INCLUDE_DIRS ${XKB_INCLUDE_DIR})
    set(XKB_VERSION ${PKG_XKB_VERSION})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(XKB
        FOUND_VAR
            XKB_FOUND
        REQUIRED_VARS
            XKB_LIBRARY
            XKB_INCLUDE_DIR
        VERSION_VAR
            XKB_VERSION
    )

    if(XKB_FOUND AND NOT TARGET XKB::XKB)
        add_library(XKB::XKB UNKNOWN IMPORTED)
        set_target_properties(XKB::XKB PROPERTIES
            IMPORTED_LOCATION "${XKB_LIBRARY}"
            INTERFACE_COMPILE_OPTIONS "${XKB_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${XKB_INCLUDE_DIR}"
        )
    endif()

else()
    message(STATUS "FindXKB.cmake cannot find XKB on Windows systems.")
    set(XKB_FOUND FALSE)
endif()

include(FeatureSummary)
set_package_properties(XKB PROPERTIES
    URL "http://xkbcommon.org"
    DESCRIPTION "XKB API common to servers and clients."
)
