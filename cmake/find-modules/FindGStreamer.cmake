# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later

#[=======================================================================[.rst:
FindGStreamer
-------

Finds the GStreamer library via ``pkg-config``. Creates the ``GStreamer::GStreamer``
imported target and per-component ``GStreamer::<plugin>`` targets.

Imported Targets
^^^^^^^^^^^^^^^^

``GStreamer::GStreamer``
  Core GStreamer library (plus transitive deps via ``GStreamer::deps``).

Result Variables
^^^^^^^^^^^^^^^^

``GStreamer_FOUND``
  True if the system has the GStreamer library.
``GStreamer_VERSION``
  The version of the GStreamer library which was found.

Configuration Variables
^^^^^^^^^^^^^^^^^^^^^^^

``GStreamer_ROOT_DIR``
  Installation prefix of the GStreamer SDK (required).

``GStreamer_USE_STATIC_LIBS``
  Set to ON to force the use of the static libraries. Default is OFF.

#]=======================================================================]

if (GStreamer_FOUND)
    return()
endif()

if (NOT GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
    message(FATAL_ERROR "GStreamer_ROOT_DIR must be set to a valid directory before including FindGStreamer "
        "(current value: '${GStreamer_ROOT_DIR}')")
endif()

if (NOT DEFINED GStreamer_USE_STATIC_LIBS)
    set(GStreamer_USE_STATIC_LIBS OFF)
endif()

# Only set pkg-config paths if the orchestrator (FindQGCGStreamer) hasn't already
# configured them via PKG_CONFIG_LIBDIR / _gst_configure_pkg_config.
if (NOT DEFINED ENV{PKG_CONFIG_LIBDIR} OR "$ENV{PKG_CONFIG_LIBDIR}" STREQUAL "")
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        set(ENV{PKG_CONFIG_PATH} "${GStreamer_ROOT_DIR}/lib/pkgconfig;${GStreamer_ROOT_DIR}/lib/gstreamer-1.0/pkgconfig;${GStreamer_ROOT_DIR}/lib/gio/modules/pkgconfig")
        # Block pkgconf's forced relocation for non-lib/pkgconfig modules on Windows
        # https://github.com/pkgconf/pkgconf/commit/dcf529b83d621ed09e99e41fc35fdffd068bd87a
        set(ENV{PKG_CONFIG_DONT_DEFINE_PREFIX} 1)
    else()
        set(ENV{PKG_CONFIG_PATH} "${GStreamer_ROOT_DIR}/lib/pkgconfig:${GStreamer_ROOT_DIR}/lib/gstreamer-1.0/pkgconfig:${GStreamer_ROOT_DIR}/lib/gio/modules/pkgconfig")
    endif()
endif()

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
        # CMake splits "-framework Foo" into separate args; reassemble them here
        set(new_ldflags)
        set(assemble_framework FALSE)
        foreach(_arg IN LISTS ${PC_STATIC_LDFLAGS_OTHER})
            if (assemble_framework)
                set(assemble_framework FALSE)
                find_library(GST_${_arg}_LIB ${_arg} REQUIRED)
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
        if (assemble_framework)
            message(WARNING "GStreamer: trailing -framework with no name in ${${PC_STATIC_LDFLAGS_OTHER}}")
        endif()
        set_target_properties(${GST_TARGET} PROPERTIES
            INTERFACE_LINK_OPTIONS "${new_ldflags}"
        )
    else()
        set_target_properties(${GST_TARGET} PROPERTIES
            INTERFACE_LINK_OPTIONS "${${PC_STATIC_LDFLAGS_OTHER}}"
        )
    endif()
endmacro()

find_package(PkgConfig REQUIRED)

# Query only gstreamer-1.0 core here. Extra deps (base, gl, video, etc.) are
# each queried individually in _gst_create_component_target below, which avoids
# the fragile combined query that fails if any single .pc file is missing.
if(GStreamer_FIND_VERSION)
    pkg_check_modules(PC_GStreamer REQUIRED gstreamer-1.0>=${GStreamer_FIND_VERSION})
else()
    pkg_check_modules(PC_GStreamer REQUIRED gstreamer-1.0)
endif()

if(PC_GStreamer_VERSION)
    set(GStreamer_VERSION "${PC_GStreamer_VERSION}")
endif()

if(GStreamer_DEBUG)
    message(STATUS "[GstFind] PC_GStreamer_LIBRARIES = ${PC_GStreamer_LIBRARIES}")
    message(STATUS "[GstFind] PC_GStreamer_STATIC_LIBRARIES = ${PC_GStreamer_STATIC_LIBRARIES}")
    message(STATUS "[GstFind] PC_GStreamer_STATIC_LIBRARY_DIRS = ${PC_GStreamer_STATIC_LIBRARY_DIRS}")
    message(STATUS "[GstFind] GSTREAMER_PLUGINS = ${GSTREAMER_PLUGINS}")
    message(STATUS "[GstFind] GSTREAMER_APIS = ${GSTREAMER_APIS}")
endif()

# _gst_IGNORED_SYSTEM_LIBRARIES and _gst_SRT_REGEX_PATCH are defined in GStreamerHelpers.cmake

if(PC_GStreamer_FOUND AND (NOT TARGET GStreamer::GStreamer))
    add_library(GStreamer::GStreamer INTERFACE IMPORTED)
    add_library(GStreamer::deps INTERFACE IMPORTED)

    if (GStreamer_USE_STATIC_LIBS)
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
        _gst_resolve_and_link_libraries(GStreamer::GStreamer INTERFACE PC_GStreamer_LIBRARIES PC_GStreamer_STATIC_LIBRARY_DIRS)
        _gst_resolve_and_link_libraries(GStreamer::deps INTERFACE PC_GStreamer_STATIC_LIBRARIES PC_GStreamer_STATIC_LIBRARY_DIRS HIDE)
    else()
        set_target_properties(GStreamer::GStreamer PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_CFLAGS_OTHER}"
            INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_INCLUDE_DIRS}"
            INTERFACE_LINK_OPTIONS "${PC_GStreamer_LDFLAGS_OTHER}"
        )
        set_target_properties(GStreamer::deps PROPERTIES
            INTERFACE_LINK_LIBRARIES "${PC_GStreamer_LINK_LIBRARIES}"
        )
    endif()

    target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::deps)
endif()

function(_gst_create_component_target _gst_PLUGIN _gst_PC_NAME)
    if (TARGET GStreamer::${_gst_PLUGIN})
        return()
    endif()

    if (GStreamer_FIND_REQUIRED_${_gst_PLUGIN})
        set(_gst_PLUGIN_REQUIRED REQUIRED)
    else()
        set(_gst_PLUGIN_REQUIRED)
    endif()

    pkg_check_modules(PC_GStreamer_${_gst_PLUGIN} ${_gst_PLUGIN_REQUIRED} "${_gst_PC_NAME}")

    set(GStreamer_${_gst_PLUGIN}_FOUND "${PC_GStreamer_${_gst_PLUGIN}_FOUND}" PARENT_SCOPE)
    if (NOT PC_GStreamer_${_gst_PLUGIN}_FOUND)
        if(GStreamer_DEBUG)
            message(STATUS "[GstFind] Component ${_gst_PLUGIN} (${_gst_PC_NAME}): NOT FOUND")
        endif()
        return()
    endif()
    if(GStreamer_DEBUG)
        message(STATUS "[GstFind] Component ${_gst_PLUGIN} (${_gst_PC_NAME}): FOUND, STATIC_LIBS=${PC_GStreamer_${_gst_PLUGIN}_STATIC_LIBRARIES}")
    endif()

    set(_pc "PC_GStreamer_${_gst_PLUGIN}")
    add_library(GStreamer::${_gst_PLUGIN} INTERFACE IMPORTED GLOBAL)

    # Select static or shared pkg-config variable sets
    if (GStreamer_USE_STATIC_LIBS)
        set(_inc_var "${_pc}_STATIC_INCLUDE_DIRS")
        set(_cflags_var "${_pc}_STATIC_CFLAGS_OTHER")
        set(_ldflags_var "${_pc}_STATIC_LDFLAGS_OTHER")
    else()
        set(_inc_var "${_pc}_INCLUDE_DIRS")
        set(_cflags_var "${_pc}_CFLAGS_OTHER")
        set(_ldflags_var "${_pc}_LDFLAGS_OTHER")
    endif()

    _gst_filter_missing_directories(${_inc_var})
    set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
        INTERFACE_COMPILE_OPTIONS "${${_cflags_var}}"
    )
    if (${_inc_var})
        set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${${_inc_var}}"
        )
    endif()

    if (GStreamer_USE_STATIC_LIBS)
        _gst_apply_frameworks(${_ldflags_var} GStreamer::${_gst_PLUGIN})
        _gst_resolve_and_link_libraries(GStreamer::${_gst_PLUGIN} INTERFACE ${_pc}_STATIC_LIBRARIES ${_pc}_STATIC_LIBRARY_DIRS)
    else()
        set_target_properties(GStreamer::${_gst_PLUGIN} PROPERTIES
            INTERFACE_LINK_OPTIONS "${${_ldflags_var}}"
            INTERFACE_LINK_LIBRARIES "${${_pc}_LINK_LIBRARIES}"
        )
    endif()
endfunction()

foreach(_gst_PLUGIN IN LISTS GSTREAMER_PLUGINS)
    _gst_create_component_target(${_gst_PLUGIN} "gst${_gst_PLUGIN}")
endforeach()

foreach(_gst_PLUGIN IN LISTS GSTREAMER_APIS)
    string(REGEX REPLACE "^api_(.+)" "\\1" _gst_PLUGIN_PC "${_gst_PLUGIN}")
    string(REPLACE "_" "-" _gst_PLUGIN_PC "${_gst_PLUGIN_PC}")
    _gst_create_component_target(${_gst_PLUGIN} "gstreamer-${_gst_PLUGIN_PC}-1.0")
endforeach()

# Link API component targets into the umbrella so consumers get the full set
# of includes and libraries (rtsp, video, gl, etc.) transitively.
if(TARGET GStreamer::GStreamer)
    foreach(_gst_API IN LISTS GSTREAMER_APIS)
        if(TARGET GStreamer::${_gst_API})
            target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::${_gst_API})
        endif()
    endforeach()
    if(GStreamer_USE_STATIC_LIBS)
        foreach(_gst_PLUGIN IN LISTS GSTREAMER_PLUGINS)
            if(TARGET GStreamer::${_gst_PLUGIN})
                target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::${_gst_PLUGIN})
            endif()
        endforeach()
    endif()
endif()

if (DEFINED ENV{PKG_CONFIG_DONT_DEFINE_PREFIX})
    set(ENV{PKG_CONFIG_DONT_DEFINE_PREFIX})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GStreamer
    REQUIRED_VARS
        GStreamer_VERSION
        GStreamer_ROOT_DIR
    VERSION_VAR GStreamer_VERSION
    HANDLE_VERSION_RANGE
    HANDLE_COMPONENTS
)
