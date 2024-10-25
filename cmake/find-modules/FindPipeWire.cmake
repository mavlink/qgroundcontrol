#.rst:
# FindPipeWire
# -------
#
# Try to find PipeWire on a Unix system.
#
# This will define the following variables:
#
# ``PipeWire_FOUND``
#     True if (the requested version of) PipeWire is available
# ``PipeWire_VERSION``
#     The version of PipeWire
# ``PipeWire_LIBRARIES``
#     This can be passed to target_link_libraries() instead of the ``PipeWire::PipeWire``
#     target
# ``PipeWire_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if the target is not
#     used for linking
# ``PipeWire_DEFINITIONS``
#     This should be passed to target_compile_options() if the target is not
#     used for linking
#
# If ``PipeWire_FOUND`` is TRUE, it will also define the following imported target:
#
# ``PipeWire::PipeWire``
#     The PipeWire library
#
# In general we recommend using the imported target, as it is easier to use.
# Bear in mind, however, that if the target is in the link interface of an
# exported library, it must be made available by the package config file.

#=============================================================================
# Copyright 2014 Alex Merry <alex.merry@kde.org>
# Copyright 2014 Martin Gräßlin <mgraesslin@kde.org>
# Copyright 2018-2020 Jan Grulich <jgrulich@redhat.com>
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

# Use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    pkg_search_module(PKG_PipeWire QUIET libpipewire-0.3)
    pkg_search_module(PKG_Spa QUIET libspa-0.2)
endif()

set(PipeWire_DEFINITIONS "${PKG_PipeWire_CFLAGS}" "${PKG_Spa_CFLAGS}")
set(PipeWire_VERSION "${PKG_PipeWire_VERSION}")

find_path(PipeWire_INCLUDE_DIRS
    NAMES
        pipewire/pipewire.h
    HINTS
        ${PKG_PipeWire_INCLUDE_DIRS}
        ${PKG_PipeWire_INCLUDE_DIRS}/pipewire-0.3
)

find_path(Spa_INCLUDE_DIRS
    NAMES
        spa/param/props.h
    HINTS
        ${PKG_Spa_INCLUDE_DIRS}
        ${PKG_Spa_INCLUDE_DIRS}/spa-0.2
)

find_library(PipeWire_LIBRARIES
    NAMES
        pipewire-0.3
    HINTS
        ${PKG_PipeWire_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PipeWire
    FOUND_VAR
        PipeWire_FOUND
    REQUIRED_VARS
        PipeWire_LIBRARIES
        PipeWire_INCLUDE_DIRS
        Spa_INCLUDE_DIRS
    VERSION_VAR
        PipeWire_VERSION
)

if(PipeWire_FOUND AND NOT TARGET PipeWire::PipeWire)
    add_library(PipeWire::PipeWire UNKNOWN IMPORTED)
    set_target_properties(PipeWire::PipeWire PROPERTIES
        IMPORTED_LOCATION "${PipeWire_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${PipeWire_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${PipeWire_INCLUDE_DIRS};${Spa_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(PipeWire_LIBRARIES PipeWire_INCLUDE_DIRS)

include(FeatureSummary)
set_package_properties(PipeWire PROPERTIES
    URL "https://www.pipewire.org"
    DESCRIPTION "PipeWire - multimedia processing"
)
