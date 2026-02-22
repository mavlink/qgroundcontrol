# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later

#[=======================================================================[.rst:
FindGStreamer
-------

Finds the GStreamer library. Requires ``pkg-config`` to be installed.

Configuration
^^^^^^^^^^^^^

This module can be configured with the following variables:

``GStreamer_USE_STATIC_LIBS``
  Link against GStreamer statically (see below).

Imported Targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` targets:

``GStreamer::GStreamer``
  The GStreamer library.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``GStreamer_FOUND``
  True if the system has the GStreamer library.
``GStreamer_VERSION``
  The version of the GStreamer library which was found.
``GStreamer_INCLUDE_DIRS``
  Include directories needed to use GStreamer.
``GStreamer_LIBRARIES``
  Libraries needed to link to GStreamer.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``GStreamer_INCLUDE_DIR``
  The directory containing ``gst/gstversion.h``.
``GStreamer_LIBRARY``
  The path to the GStreamer library.

Configuration Variables
^^^^^^^^^^^^^^^

Setting the following variables is required, depending on the operating system:

``GStreamer_ROOT_DIR``
  Installation prefix of the GStreamer SDK.

``GStreamer_USE_STATIC_LIBS``
  Set to ON to force the use of the static libraries. Default is OFF.

``GStreamer_EXTRA_DEPS``
  pkg-config names of the extra dependencies that will be included whenever linking against GStreamer.

#]=======================================================================]

if (GStreamer_FOUND)
    return()
endif()

if (NOT DEFINED GStreamer_ROOT_DIR AND DEFINED GSTREAMER_ROOT)
    set(GStreamer_ROOT_DIR ${GSTREAMER_ROOT})
endif()

if (NOT GStreamer_ROOT_DIR)
    set(GStreamer_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../")
    cmake_path(NORMAL_PATH GStreamer_ROOT_DIR)
    message(STATUS "FindGStreamer: GStreamer_ROOT_DIR not set, defaulting to ${GStreamer_ROOT_DIR}")
endif()

if (NOT EXISTS "${GStreamer_ROOT_DIR}")
    message(FATAL_ERROR "The directory GStreamer_ROOT_DIR=${GStreamer_ROOT_DIR} does not exist")
endif()

if (NOT DEFINED GStreamer_USE_STATIC_LIBS)
    set(GStreamer_USE_STATIC_LIBS OFF)
endif()

# Prepend GStreamer paths to existing PKG_CONFIG_PATH so both SDK and
# user-specified .pc files are discoverable.
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_gst_pc_sep ";")
else()
    set(_gst_pc_sep ":")
endif()
set(_gst_pc_paths "${GStreamer_ROOT_DIR}/lib/pkgconfig${_gst_pc_sep}${GStreamer_ROOT_DIR}/lib/gstreamer-1.0/pkgconfig${_gst_pc_sep}${GStreamer_ROOT_DIR}/lib/gio/modules/pkgconfig")
if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
    set(ENV{PKG_CONFIG_PATH} "${_gst_pc_paths}${_gst_pc_sep}$ENV{PKG_CONFIG_PATH}")
else()
    set(ENV{PKG_CONFIG_PATH} "${_gst_pc_paths}")
endif()
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # Block pkgconf's forced relocation for non-lib/pkgconfig modules on Windows
    # https://github.com/pkgconf/pkgconf/commit/dcf529b83d621ed09e99e41fc35fdffd068bd87a
    set(ENV{PKG_CONFIG_DONT_DEFINE_PREFIX} 1)
endif()

if (NOT DEFINED GStreamer_EXTRA_DEPS)
    set(GStreamer_EXTRA_DEPS)
    if (DEFINED GSTREAMER_EXTRA_DEPS)
        set(GStreamer_EXTRA_DEPS ${GSTREAMER_EXTRA_DEPS})
    endif()
endif()

# Find libraries, preferring static. Falls back to shared/stub if unavailable.
function(_gst_find_library LOCAL_LIB GST_LOCAL_LIB)
    if (DEFINED ${GST_LOCAL_LIB})
        return()
    endif()

    if (APPLE)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".dylib" ".so" ".tbd")
        set(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    elseif (UNIX)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".so")
        set(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    elseif(MSVC)
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".a")
        set(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib")
        set(CMAKE_FIND_LIBRARY_PREFIXES "" "lib")
    endif()

    if ("${LOCAL_LIB}" IN_LIST _gst_IGNORED_SYSTEM_LIBRARIES)
        set(${GST_LOCAL_LIB} ${LOCAL_LIB} CACHE INTERNAL "")
    else()
        find_library(${GST_LOCAL_LIB}
            ${LOCAL_LIB}
            HINTS ${ARGN}
            NO_DEFAULT_PATH
            NO_CMAKE_FIND_ROOT_PATH
        )
        set(${GST_LOCAL_LIB} "${${GST_LOCAL_LIB}}" PARENT_SCOPE)
    endif()
endfunction()

function(_gst_apply_link_libraries HIDE PC_LIBRARIES PC_HINTS GST_TARGET)
    if (APPLE AND ${HIDE})
        target_link_directories(${GST_TARGET} INTERFACE
            ${${PC_HINTS}}
        )
    endif()
    foreach(_all_LOCAL_LIB IN LISTS ${PC_LIBRARIES})
        if (_all_LOCAL_LIB MATCHES "${_gst_SRT_REGEX_PATCH}")
            string(REGEX REPLACE "${_gst_SRT_REGEX_PATCH}" "\\1" _all_LOCAL_LIB "${_all_LOCAL_LIB}")
        endif()
        string(MAKE_C_IDENTIFIER "_gst_${_all_LOCAL_LIB}" _all_GST_LOCAL_LIB)
        if (NOT DEFINED ${_all_GST_LOCAL_LIB} OR "${${_all_GST_LOCAL_LIB}}" STREQUAL "")
            _gst_find_library(${_all_LOCAL_LIB} ${_all_GST_LOCAL_LIB} ${${PC_HINTS}})
        endif()
        if ("${${_all_GST_LOCAL_LIB}}" IN_LIST _gst_IGNORED_SYSTEM_LIBRARIES)
            target_link_libraries(${GST_TARGET} INTERFACE
                ${${_all_GST_LOCAL_LIB}})
        elseif (APPLE AND ${HIDE})
            cmake_path(GET ${_all_GST_LOCAL_LIB} STEM LAST_ONLY _all_LOCAL_FILE)
            string(REGEX REPLACE "^lib" "" _all_LOCAL_FILE "${_all_LOCAL_FILE}")
            target_link_libraries(${GST_TARGET} INTERFACE
                "-hidden-l${_all_LOCAL_FILE}")
        elseif(UNIX AND ${HIDE})
            target_link_libraries(${GST_TARGET} INTERFACE
                -Wl,--exclude-libs,${${_all_GST_LOCAL_LIB}})
        else()
            target_link_libraries(${GST_TARGET} INTERFACE
                ${${_all_GST_LOCAL_LIB}})
        endif()
    endforeach()
endfunction()

macro(_gst_filter_missing_directories GST_INCLUDE_DIRS)
    set(_gst_include_dirs)
    foreach(DIR IN LISTS ${GST_INCLUDE_DIRS})
        string(MAKE_C_IDENTIFIER "${DIR}" _gst_dir_id)
        if (DEFINED _gst_exists_${_gst_dir_id})
            if (_gst_exists_${_gst_dir_id})
                list(APPEND _gst_include_dirs "${DIR}")
            endif()
        elseif (EXISTS "${DIR}")
            list(APPEND _gst_include_dirs "${DIR}")
            set(_gst_exists_${_gst_dir_id} TRUE)
        else()
            message(WARNING "Skipping missing include folder ${DIR}.")
            set(_gst_exists_${_gst_dir_id} FALSE)
        endif()
    endforeach()
    set(${GST_INCLUDE_DIRS} "${_gst_include_dirs}")
endmacro()

macro(_gst_apply_frameworks PC_STATIC_LDFLAGS_OTHER GST_TARGET)
    if (APPLE)
        # CMake splits LDFLAGS_OTHER by spaces, breaking "-framework Foo" pairs.
        set(new_ldflags)
        set(assemble_framework FALSE)
        foreach(_arg IN LISTS ${PC_STATIC_LDFLAGS_OTHER})
            if (assemble_framework)
                set(assemble_framework FALSE)
                find_library(GST_${_arg}_LIB ${_arg})
                target_link_libraries(${GST_TARGET}
                    INTERFACE
                        "${GST_${_arg}_LIB}"
                )
            elseif (_arg STREQUAL "-framework")
                set(assemble_framework TRUE)
            else()
                set(assemble_framework FALSE)
                list(APPEND new_ldflags "${_arg}")
            endif()
        endforeach()
        set_target_properties(${GST_TARGET} PROPERTIES
            INTERFACE_LINK_OPTIONS "${new_ldflags}"
        )
    else()
        set_target_properties(${GST_TARGET} PROPERTIES
            INTERFACE_LINK_OPTIONS "${${PC_STATIC_LDFLAGS_OTHER}}"
        )
    endif()
endmacro()

if(NOT PKG_CONFIG_FOUND)
    find_package(PkgConfig REQUIRED)
endif()

if(GStreamer_FIND_QUIETLY)
    pkg_check_modules(PC_GStreamer QUIET gstreamer-1.0 ${GStreamer_EXTRA_DEPS})
else()
    pkg_check_modules(PC_GStreamer gstreamer-1.0 ${GStreamer_EXTRA_DEPS})
endif()

set(GStreamer_VERSION "${PC_GStreamer_VERSION}")

if(NOT GStreamer_VERSION AND PC_GStreamer_INCLUDE_DIRS)
    foreach(_inc_dir IN LISTS PC_GStreamer_INCLUDE_DIRS)
        set(_version_header "${_inc_dir}/gst/gstversion.h")
        if(EXISTS "${_version_header}")
            file(STRINGS "${_version_header}" _version_major REGEX "^#define GST_VERSION_MAJOR")
            file(STRINGS "${_version_header}" _version_minor REGEX "^#define GST_VERSION_MINOR")
            file(STRINGS "${_version_header}" _version_micro REGEX "^#define GST_VERSION_MICRO")
            if(_version_major AND _version_minor AND _version_micro)
                string(REGEX REPLACE ".*GST_VERSION_MAJOR[^0-9]*([0-9]+).*" "\\1" _major "${_version_major}")
                string(REGEX REPLACE ".*GST_VERSION_MINOR[^0-9]*([0-9]+).*" "\\1" _minor "${_version_minor}")
                string(REGEX REPLACE ".*GST_VERSION_MICRO[^0-9]*([0-9]+).*" "\\1" _micro "${_version_micro}")
                set(GStreamer_VERSION "${_major}.${_minor}.${_micro}")
                break()
            endif()
        endif()
    endforeach()
endif()

find_path(GStreamer_INCLUDE_DIR
    NAMES gst/gstversion.h
    PATHS ${PC_GStreamer_INCLUDE_DIRS}
    PATH_SUFFIXES gstreamer-1.0
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
)

find_library(GStreamer_LIBRARY
    NAMES gstreamer-1.0
    PATHS ${PC_GStreamer_LIBRARY_DIRS}
    NO_DEFAULT_PATH
    NO_CMAKE_FIND_ROOT_PATH
)

# Android: Ignore these libraries when constructing the IMPORTED_LOCATION
set(_gst_IGNORED_SYSTEM_LIBRARIES c c++ unwind m dl atomic)
if (ANDROID)
    list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES log GLESv2 EGL OpenSLES android vulkan)
elseif(APPLE)
    list(APPEND _gst_IGNORED_SYSTEM_LIBRARIES iconv resolv System)
endif()

# Normalize library flags coming from srt/haisrt
# https://github.com/Haivision/srt/commit/b90b64d26f850fb0efcc4cdd8b31cbf74bd4db0c
set(_gst_SRT_REGEX_PATCH "^:lib(.+)\\.(a|so|lib|dylib)$")

if(PC_GStreamer_FOUND AND (NOT TARGET GStreamer::GStreamer))
    # INTERFACE target — FindPkgConfig cannot distinguish shared from static
    # in xxx_STATIC_LINK_LIBRARIES, so libraries are resolved manually.
    add_library(GStreamer::GStreamer INTERFACE IMPORTED)

    if(MACOS AND GStreamer_USE_FRAMEWORK AND GSTREAMER_FRAMEWORK)
        set_target_properties(GStreamer::GStreamer PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GSTREAMER_FRAMEWORK}/Headers"
            INTERFACE_LINK_LIBRARIES "${GSTREAMER_FRAMEWORK}"
        )
        target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_MACOS_FRAMEWORK)
    elseif (GStreamer_USE_STATIC_LIBS)
        _gst_filter_missing_directories(PC_GStreamer_STATIC_INCLUDE_DIRS)
        set_target_properties(GStreamer::GStreamer PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_STATIC_CFLAGS_OTHER}"
        )
        if (PC_GStreamer_STATIC_INCLUDE_DIRS)
            set_target_properties(GStreamer::GStreamer PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_STATIC_INCLUDE_DIRS}"
            )
        endif()
        _gst_apply_frameworks(PC_GStreamer_STATIC_LDFLAGS_OTHER GStreamer::GStreamer)
    else()
        set_target_properties(GStreamer::GStreamer PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_CFLAGS_OTHER}"
            INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_INCLUDE_DIRS}"
            INTERFACE_LINK_OPTIONS "${PC_GStreamer_LDFLAGS_OTHER}"
        )
    endif()

    if(NOT (MACOS AND GStreamer_USE_FRAMEWORK AND GSTREAMER_FRAMEWORK))
        add_library(GStreamer::deps INTERFACE IMPORTED)

        if (NOT GStreamer_USE_STATIC_LIBS)
            set_target_properties(GStreamer::deps PROPERTIES
                INTERFACE_LINK_LIBRARIES "${PC_GStreamer_LINK_LIBRARIES}"
            )

        else()
            # stock pkg-config lacks --maximum-traverse-depth, so cache
            # shared-variant library paths for _gst_apply_link_libraries.
            # Linked via GStreamer::deps to avoid duplicates.
            foreach(LOCAL_LIB IN LISTS PC_GStreamer_LIBRARIES)
                # list(TRANSFORM REPLACE) is of no use here
                # https://gitlab.kitware.com/cmake/cmake/-/issues/16899
                if (LOCAL_LIB MATCHES "${_gst_SRT_REGEX_PATCH}")
                    string(REGEX REPLACE "${_gst_SRT_REGEX_PATCH}" "\\1" LOCAL_LIB "${LOCAL_LIB}")
                endif()
                string(MAKE_C_IDENTIFIER "_gst_${LOCAL_LIB}" GST_LOCAL_LIB)
                if (NOT DEFINED ${GST_LOCAL_LIB} OR "${${GST_LOCAL_LIB}}" STREQUAL "")
                    _gst_find_library(${LOCAL_LIB} ${GST_LOCAL_LIB} ${PC_GStreamer_STATIC_LIBRARY_DIRS})
                endif()
            endforeach()

            _gst_apply_link_libraries(ON PC_GStreamer_STATIC_LIBRARIES PC_GStreamer_STATIC_LIBRARY_DIRS GStreamer::deps)
        endif()

        target_link_libraries(GStreamer::GStreamer
            INTERFACE
                GStreamer::deps
        )
    endif()
endif()

foreach(_gst_PLUGIN IN LISTS GSTREAMER_PLUGINS)
    if (TARGET GStreamer::${_gst_PLUGIN})
        continue()
    endif()

    pkg_check_modules(PC_GStreamer_${_gst_PLUGIN} QUIET "gst${_gst_PLUGIN}")

    set(GStreamer_${_gst_PLUGIN}_FOUND "${PC_GStreamer_${_gst_PLUGIN}_FOUND}")
    if (NOT GStreamer_${_gst_PLUGIN}_FOUND)
        continue()
    endif()

    add_library(GStreamer::${_gst_PLUGIN} INTERFACE IMPORTED)
    if (GStreamer_USE_STATIC_LIBS)
        _gst_filter_missing_directories(PC_GStreamer_${_gst_PLUGIN}_STATIC_INCLUDE_DIRS)
        set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_${_gst_PLUGIN}_STATIC_CFLAGS_OTHER}"
        )
        if (PC_GStreamer_${_gst_PLUGIN}_STATIC_INCLUDE_DIRS)
            set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_${_gst_PLUGIN}_STATIC_INCLUDE_DIRS}"
            )
        endif()
        _gst_apply_frameworks(PC_GStreamer_${_gst_PLUGIN}_STATIC_LDFLAGS_OTHER GStreamer::${_gst_PLUGIN})
        _gst_apply_link_libraries(OFF PC_GStreamer_${_gst_PLUGIN}_STATIC_LIBRARIES PC_GStreamer_${_gst_PLUGIN}_STATIC_LIBRARY_DIRS GStreamer::${_gst_PLUGIN})
    else()
        _gst_filter_missing_directories(PC_GStreamer_${_gst_PLUGIN}_INCLUDE_DIRS)
        set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_${_gst_PLUGIN}_CFLAGS_OTHER}"
            INTERFACE_LINK_OPTIONS "${PC_GStreamer_${_gst_PLUGIN}_LDFLAGS_OTHER}"
            INTERFACE_LINK_LIBRARIES "${PC_GStreamer_${_gst_PLUGIN}_LINK_LIBRARIES}"
        )
        if (PC_GStreamer_${_gst_PLUGIN}_INCLUDE_DIRS)
            set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_${_gst_PLUGIN}_INCLUDE_DIRS}"
            )
        endif()
    endif()
endforeach()

foreach(_gst_PLUGIN IN LISTS GSTREAMER_APIS)
    if (TARGET GStreamer::${_gst_PLUGIN})
        continue()
    endif()

    string(REGEX REPLACE "^api_(.+)" "\\1" _gst_PLUGIN_PC "${_gst_PLUGIN}")
    string(REPLACE "_" "-" _gst_PLUGIN_PC "${_gst_PLUGIN_PC}")

    pkg_check_modules(PC_GStreamer_${_gst_PLUGIN} QUIET "gstreamer-${_gst_PLUGIN_PC}-1.0")

    set(GStreamer_${_gst_PLUGIN}_FOUND "${PC_GStreamer_${_gst_PLUGIN}_FOUND}")
    if (NOT GStreamer_${_gst_PLUGIN}_FOUND)
        continue()
    endif()

    add_library(GStreamer::${_gst_PLUGIN} INTERFACE IMPORTED)
    if (GStreamer_USE_STATIC_LIBS)
        _gst_filter_missing_directories(PC_GStreamer_${_gst_PLUGIN}_STATIC_INCLUDE_DIRS)
        set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_${_gst_PLUGIN}_STATIC_CFLAGS_OTHER}"
        )
        if (PC_GStreamer_${_gst_PLUGIN}_STATIC_INCLUDE_DIRS)
            set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_${_gst_PLUGIN}_STATIC_INCLUDE_DIRS}"
            )
        endif()
        _gst_apply_frameworks(PC_GStreamer_${_gst_PLUGIN}_STATIC_LDFLAGS_OTHER GStreamer::${_gst_PLUGIN})
        _gst_apply_link_libraries(OFF PC_GStreamer_${_gst_PLUGIN}_STATIC_LIBRARIES PC_GStreamer_${_gst_PLUGIN}_STATIC_LIBRARY_DIRS GStreamer::${_gst_PLUGIN})
    else()
        _gst_filter_missing_directories(PC_GStreamer_${_gst_PLUGIN}_INCLUDE_DIRS)
        set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_${_gst_PLUGIN}_CFLAGS_OTHER}"
            INTERFACE_LINK_OPTIONS "${PC_GStreamer_${_gst_PLUGIN}_LDFLAGS_OTHER}"
            INTERFACE_LINK_LIBRARIES "${PC_GStreamer_${_gst_PLUGIN}_LINK_LIBRARIES}"
        )
        if (PC_GStreamer_${_gst_PLUGIN}_INCLUDE_DIRS)
            set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_${_gst_PLUGIN}_INCLUDE_DIRS}"
            )
        endif()
    endif()
endforeach()

set(GStreamer_VERSION "${GStreamer_VERSION}" CACHE STRING "GStreamer version" FORCE)

include(FindPackageHandleStandardArgs)
# Suppress "package name mismatch" warning when included from FindGStreamerQGC
set(FPHSA_NAME_MISMATCHED TRUE)
find_package_handle_standard_args(GStreamer
    REQUIRED_VARS
        PC_GStreamer_FOUND
        GStreamer_LIBRARY
        GStreamer_INCLUDE_DIR
    VERSION_VAR GStreamer_VERSION
    HANDLE_VERSION_RANGE
)
unset(FPHSA_NAME_MISMATCHED)
