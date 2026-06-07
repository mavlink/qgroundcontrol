#.rst:
# FindLibdrm
# -------
#
# Try to find libdrm on a Unix system.
#
# This will define the following variables:
#
# ``Libdrm_FOUND``
#     True if (the requested version of) libdrm is available
# ``Libdrm_VERSION``
#     The version of libdrm
# ``Libdrm_LIBRARIES``
#     This can be passed to target_link_libraries() instead of the ``Libdrm::Libdrm``
#     target
# ``Libdrm_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if the target is not
#     used for linking
# ``Libdrm_DEFINITIONS``
#     This should be passed to target_compile_options() if the target is not
#     used for linking
#
# If ``Libdrm_FOUND`` is TRUE, it will also define the following imported target:
#
# ``Libdrm::Libdrm``
#     The libdrm library
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
    message(FATAL_ERROR "CMake 2.8.12 is required by FindLibdrm.cmake")
endif()
if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
    message(AUTHOR_WARNING "Your project should require at least CMake 2.8.12 to use FindLibdrm.cmake")
endif()

if(NOT WIN32)
    if (VXWORKS)
        set(_vxworks_lib_search_path "$ENV{WIND_CC_SYSROOT}/usr/lib/common/")
        set(_vxworks_lib_drm_library_name gfxLibDRM)
    endif()

    find_package(PkgConfig QUIET)
    pkg_check_modules(PKG_Libdrm QUIET libdrm ${_vxworks_lib_drm_library_name})

    set(Libdrm_DEFINITIONS ${PKG_Libdrm_CFLAGS_OTHER})
    set(Libdrm_VERSION ${PKG_Libdrm_VERSION})

    find_path(Libdrm_INCLUDE_DIR
        NAMES
            xf86drm.h
        HINTS
            ${PKG_Libdrm_INCLUDE_DIRS}
        PATH_SUFFIXES
            libdrm
    )
    find_library(Libdrm_LIBRARY
        NAMES
            drm
            ${_vxworks_lib_drm_library_name}
        HINTS
            ${PKG_Libdrm_LIBRARY_DIRS}
            "${_vxworks_lib_search_path}"
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Libdrm
        FOUND_VAR
            Libdrm_FOUND
        REQUIRED_VARS
            Libdrm_LIBRARY
            Libdrm_INCLUDE_DIR
        VERSION_VAR
            Libdrm_VERSION
    )

    if(Libdrm_FOUND AND NOT TARGET Libdrm::Libdrm)
        add_library(Libdrm::Libdrm UNKNOWN IMPORTED)
        set_target_properties(Libdrm::Libdrm PROPERTIES
            IMPORTED_LOCATION "${Libdrm_LIBRARY}"
            INTERFACE_COMPILE_OPTIONS "${Libdrm_DEFINITIONS}"
            INTERFACE_INCLUDE_DIRECTORIES "${Libdrm_INCLUDE_DIR}"
        )
        foreach(suffix libdrm drm)
            if(EXISTS "${Libdrm_INCLUDE_DIR}/${suffix}")
                set_property(TARGET Libdrm::Libdrm APPEND PROPERTY
                      INTERFACE_INCLUDE_DIRECTORIES "${Libdrm_INCLUDE_DIR}/${suffix}"
                )
            endif()
        endforeach()
    endif()

    mark_as_advanced(Libdrm_LIBRARY Libdrm_INCLUDE_DIR)

    # compatibility variables
    set(Libdrm_LIBRARIES ${Libdrm_LIBRARY})
    set(Libdrm_INCLUDE_DIRS ${Libdrm_INCLUDE_DIR} "${Libdrm_INCLUDE_DIR}/libdrm" "${Libdrm_INCLUDE_DIR}/drm")
    set(Libdrm_VERSION_STRING ${Libdrm_VERSION})

else()
    message(STATUS "FindLibdrm.cmake cannot find libdrm on Windows systems.")
    set(Libdrm_FOUND FALSE)
endif()

include(FeatureSummary)
set_package_properties(Libdrm PROPERTIES
    URL "https://wiki.freedesktop.org/dri/"
    DESCRIPTION "Userspace interface to kernel DRM services."
)
