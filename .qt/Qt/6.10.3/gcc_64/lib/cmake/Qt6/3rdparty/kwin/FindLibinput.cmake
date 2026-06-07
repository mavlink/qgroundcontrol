#.rst:
# FindLibinput
# -------
#
# Try to find libinput on a Unix system.
#
# This will define the following variables:
#
# ``Libinput_FOUND``
#     True if (the requested version of) libinput is available
# ``Libinput_VERSION``
#     The version of libinput
# ``Libinput_LIBRARIES``
#     This can be passed to target_link_libraries() instead of the ``Libinput::Libinput``
#     target
# ``Libinput_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if the target is not
#     used for linking
# ``Libinput_DEFINITIONS``
#     This should be passed to target_compile_options() if the target is not
#     used for linking
#
# If ``Libinput_FOUND`` is TRUE, it will also define the following imported target:
#
# ``Libinput::Libinput``
#     The libinput library
#
# In general we recommend using the imported target, as it is easier to use.
# Bear in mind, however, that if the target is in the link interface of an
# exported library, it must be made available by the package config file.

#=============================================================================
# Copyright 2014 Alex Merry <alex.merry@kde.org>
# Copyright 2014 Martin Gräßlin <mgraesslin@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

if(CMAKE_VERSION VERSION_LESS 2.8.12)
    message(FATAL_ERROR "CMake 2.8.12 is required by FindLibinput.cmake")
endif()
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
    message(AUTHOR_WARNING "Your project should require at least CMake 2.8.12 to use FindLibinput.cmake")
endif()

if(NOT WIN32)
    # Use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig QUIET)
    pkg_check_modules(PKG_Libinput QUIET libinput)

    set(Libinput_DEFINITIONS ${PKG_Libinput_CFLAGS_OTHER})
    set(Libinput_VERSION ${PKG_Libinput_VERSION})

    find_path(Libinput_INCLUDE_DIR
        NAMES
            libinput.h
        HINTS
            ${PKG_Libinput_INCLUDE_DIRS}
    )
    find_library(Libinput_LIBRARY
        NAMES
            input
        HINTS
            ${PKG_Libinput_LIBRARY_DIRS}
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Libinput
        FOUND_VAR
            Libinput_FOUND
        REQUIRED_VARS
            Libinput_LIBRARY
            Libinput_INCLUDE_DIR
        VERSION_VAR
            Libinput_VERSION
    )

    if(Libinput_FOUND AND NOT TARGET Libinput::Libinput)
        add_library(Libinput::Libinput UNKNOWN IMPORTED)
        set_target_properties(Libinput::Libinput PROPERTIES
            IMPORTED_LOCATION "${Libinput_LIBRARY}"
            INTERFACE_COMPILE_OPTIONS "${Libinput_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${Libinput_INCLUDE_DIR}"
        )
    endif()

    mark_as_advanced(Libinput_LIBRARY Libinput_INCLUDE_DIR)

    # compatibility variables
    set(Libinput_LIBRARIES ${Libinput_LIBRARY})
    set(Libinput_INCLUDE_DIRS ${Libinput_INCLUDE_DIR})
    set(Libinput_VERSION_STRING ${Libinput_VERSION})

else()
    message(STATUS "FindLibinput.cmake cannot find libinput on Windows systems.")
    set(Libinput_FOUND FALSE)
endif()

include(FeatureSummary)
set_package_properties(Libinput PROPERTIES
    URL "http://www.freedesktop.org/wiki/Software/libinput/"
    DESCRIPTION "Library to handle input devices in Wayland compositors and to provide a generic X.Org input driver."
)
