#.rst:
# FindWayland
# -----------
#
# Try to find Wayland.
#
# This is a component-based find module, which makes use of the COMPONENTS
# and OPTIONAL_COMPONENTS arguments to find_module.  The following components
# are available::
#
#   Client  Server  Cursor  Egl
#
# If no components are specified, this module will act as though all components
# were passed to OPTIONAL_COMPONENTS.
#
# This module will define the following variables, independently of the
# components searched for or found:
#
# ``Wayland_FOUND``
#     TRUE if (the requested version of) Wayland is available
# ``Wayland_VERSION``
#     Found Wayland version
# ``Wayland_TARGETS``
#     A list of all targets imported by this module (note that there may be more
#     than the components that were requested)
# ``Wayland_LIBRARIES``
#     This can be passed to target_link_libraries() instead of the imported
#     targets
# ``Wayland_INCLUDE_DIRS``
#     This should be passed to target_include_directories() if the targets are
#     not used for linking
# ``Wayland_DEFINITIONS``
#     This should be passed to target_compile_options() if the targets are not
#     used for linking
#
# For each searched-for components, ``Wayland_<component>_FOUND`` will be set to
# TRUE if the corresponding Wayland library was found, and FALSE otherwise.  If
# ``Wayland_<component>_FOUND`` is TRUE, the imported target
# ``Wayland::<component>`` will be defined.  This module will also attempt to
# determine ``Wayland_*_VERSION`` variables for each imported target, although
# ``Wayland_VERSION`` should normally be sufficient.
#
# In general we recommend using the imported targets, as they are easier to use
# and provide more control.  Bear in mind, however, that if any target is in the
# link interface of an exported library, it must be made available by the
# package config file.
#
# Since pre-1.0.0.

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

include(${CMAKE_CURRENT_LIST_DIR}/ECMFindModuleHelpersStub.cmake)

ecm_find_package_version_check(Wayland)

set(Wayland_known_components
    Client
    Server
    Cursor
    Egl
)
foreach(_comp ${Wayland_known_components})
    string(TOLOWER "${_comp}" _lc_comp)
    set(Wayland_${_comp}_component_deps)
    set(Wayland_${_comp}_pkg_config "wayland-${_lc_comp}")
    set(Wayland_${_comp}_lib "wayland-${_lc_comp}")
    set(Wayland_${_comp}_header "wayland-${_lc_comp}.h")
endforeach()
set(Wayland_Egl_component_deps Client)

ecm_find_package_parse_components(Wayland
    RESULT_VAR Wayland_components
    KNOWN_COMPONENTS ${Wayland_known_components}
)
ecm_find_package_handle_library_components(Wayland
    COMPONENTS ${Wayland_components}
)

# If pkg-config didn't provide us with version information,
# try to extract it from wayland-version.h
# (Note that the version from wayland-egl.pc will probably be
# the Mesa version, rather than the Wayland version, but that
# version will be ignored as we always find wayland-client.pc
# first).
if(NOT Wayland_VERSION)
    find_file(Wayland_VERSION_HEADER
        NAMES wayland-version.h
        HINTS ${Wayland_INCLUDE_DIRS}
    )
    mark_as_advanced(Wayland_VERSION_HEADER)
    if(Wayland_VERSION_HEADER)
        file(READ ${Wayland_VERSION_HEADER} _wayland_version_header_contents)
        string(REGEX REPLACE
            "^.*[ \t]+WAYLAND_VERSION[ \t]+\"([0-9.]*)\".*$"
            "\\1"
            Wayland_VERSION
            "${_wayland_version_header_contents}"
        )
        unset(_wayland_version_header_contents)
    endif()
endif()

find_package_handle_standard_args(Wayland
    FOUND_VAR
        Wayland_FOUND
    REQUIRED_VARS
        Wayland_LIBRARIES
    VERSION_VAR
        Wayland_VERSION
    HANDLE_COMPONENTS
)

include(FeatureSummary)
set_package_properties(Wayland PROPERTIES
    URL "http://wayland.freedesktop.org"
    DESCRIPTION "C library implementation of the Wayland protocol: a protocol for a compositor to talk to its clients"
)
