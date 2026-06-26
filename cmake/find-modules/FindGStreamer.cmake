# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# QGC vendoring notes:
#   Source: https://invent.kde.org/qt/qt/qtmultimedia (Qt 6.x branch)
#   This module is consumed by cmake/GStreamer/Orchestrator.cmake → platform helpers.
#   Local QGC patches (not in upstream):
#     1. _gst_resolve_and_link_libraries converted from macro to function for
#        scope hygiene; callers pass values (not names).
#     2. Pkg-config env management (PKG_CONFIG_PATH/LIBDIR/DONT_DEFINE_PREFIX)
#        moved out — every in-tree caller routes through
#        gstreamer_apply_pkgconfig_env (cmake/GStreamer/PkgConfig.cmake) before
#        find_package(GStreamer), so the upstream standalone-fallback block
#        and the trailing DONT_DEFINE_PREFIX env-reset have been deleted.
#     3. Component target generation walks GSTREAMER_APIS instead of a fixed
#        list so xcframework / mobile static-build paths can introduce new
#        components without editing this file.
#     4. Hash parsing moved out — qgc_parse_expected_hash lives in
#        cmake/modules/Download.cmake; this module no longer parses hashes.
#   When syncing from upstream, re-apply each listed patch and update this
#   block. Do NOT remove this block during sync.

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

# Short-circuit only when every currently-requested component already has a target;
# otherwise re-run so a later find_package(... COMPONENTS NewOne) can populate the
# new GStreamer::NewOne target instead of silently inheriting the cached _FOUND.
if (GStreamer_FOUND)
    set(_gst_all_present TRUE)
    foreach(_gst_c IN LISTS GStreamer_FIND_COMPONENTS)
        if (NOT TARGET GStreamer::${_gst_c} AND NOT _gst_c IN_LIST GStreamer_ABSENT_COMPONENTS)
            set(_gst_all_present FALSE)
            break()
        endif()
    endforeach()
    if (_gst_all_present)
        return()
    endif()
endif()

# Rebuilt fresh each configure; the cache entry would otherwise keep stale absences.
unset(GStreamer_ABSENT_COMPONENTS CACHE)

if (NOT GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
    message(FATAL_ERROR "GStreamer_ROOT_DIR must be set to a valid directory before including FindGStreamer "
        "(current value: '${GStreamer_ROOT_DIR}')")
endif()

if (NOT DEFINED GStreamer_USE_STATIC_LIBS)
    set(GStreamer_USE_STATIC_LIBS OFF)
endif()

# Pkg-config env (PKG_CONFIG_PATH / LIBDIR / DONT_DEFINE_PREFIX) is configured
# by the orchestrator via gstreamer_apply_pkgconfig_env() before this module
# runs — see QGC patch #2 in the vendoring header.

macro(_gst_filter_missing_directories GST_INCLUDE_DIRS)
    _gst_coalesce_existing_paths(${GST_INCLUDE_DIRS})
    set(_gst_include_dirs)
    foreach(DIR IN LISTS ${GST_INCLUDE_DIRS})
        string(MAKE_C_IDENTIFIER "${DIR}" _gst_dir_id)
        if (DEFINED _gst_exists_${_gst_dir_id} AND _gst_exists_${_gst_dir_id})
            list(APPEND _gst_include_dirs "${DIR}")
        elseif (EXISTS "${DIR}")
            list(APPEND _gst_include_dirs "${DIR}")
            set(_gst_exists_${_gst_dir_id} TRUE)
        else()
            message(WARNING "Skipping missing include folder ${DIR}.")
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
                if (GStreamer_FIND_QUIETLY)
                    find_library(GStreamer_FW_${_arg}_LIB ${_arg})
                else()
                    find_library(GStreamer_FW_${_arg}_LIB ${_arg} REQUIRED)
                endif()
                if (GStreamer_FW_${_arg}_LIB)
                    target_link_libraries(${GST_TARGET}
                        INTERFACE
                            "${GStreamer_FW_${_arg}_LIB}"
                    )
                endif()
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
        # Overwrites (not appends) INTERFACE_LINK_OPTIONS — sole writer for this target.
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
_gst_recover_split_pkgconfig_paths(PC_GStreamer
    INCLUDE_DIRS CFLAGS_OTHER
    STATIC_INCLUDE_DIRS STATIC_CFLAGS_OTHER
    LIBRARY_DIRS LDFLAGS_OTHER
    STATIC_LIBRARY_DIRS STATIC_LDFLAGS_OTHER
)

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

# _gst_IGNORED_SYSTEM_LIBRARIES, _gst_SRT_REGEX_PATCH, and _gst_resolve_and_link_libraries
# (used below) come from cmake/GStreamer/Link.cmake, included via GStreamer/Helpers before
# find_package(GStreamer). Fail loudly if that prerequisite is missing.
if(NOT COMMAND _gst_resolve_and_link_libraries)
    message(FATAL_ERROR "FindGStreamer: _gst_resolve_and_link_libraries is undefined. "
        "Include cmake/GStreamer/Link.cmake (via GStreamer/Helpers) before find_package(GStreamer).")
endif()

if(PC_GStreamer_FOUND AND (NOT TARGET GStreamer::GStreamer))
    add_library(GStreamer::GStreamer INTERFACE IMPORTED GLOBAL)
    add_library(GStreamer::deps INTERFACE IMPORTED GLOBAL)

    if (GStreamer_USE_STATIC_LIBS)
        _gst_filter_missing_directories(PC_GStreamer_STATIC_INCLUDE_DIRS)
        _gst_coalesce_existing_paths(PC_GStreamer_STATIC_LIBRARY_DIRS)
        set_target_properties(GStreamer::GStreamer PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_STATIC_CFLAGS_OTHER}"
        )
        if (PC_GStreamer_STATIC_INCLUDE_DIRS)
            set_target_properties(GStreamer::GStreamer PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_STATIC_INCLUDE_DIRS}"
            )
        endif()
        _gst_apply_frameworks(PC_GStreamer_STATIC_LDFLAGS_OTHER GStreamer::GStreamer)
        _gst_resolve_and_link_libraries(GStreamer::GStreamer INTERFACE "${PC_GStreamer_LIBRARIES}" "${PC_GStreamer_STATIC_LIBRARY_DIRS}")
        _gst_resolve_and_link_libraries(GStreamer::deps INTERFACE "${PC_GStreamer_STATIC_LIBRARIES}" "${PC_GStreamer_STATIC_LIBRARY_DIRS}" HIDE)
    else()
        _gst_filter_missing_directories(PC_GStreamer_INCLUDE_DIRS)
        _gst_coalesce_existing_paths(PC_GStreamer_LIBRARY_DIRS)
        set_target_properties(GStreamer::GStreamer PROPERTIES
            INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_CFLAGS_OTHER}"
            INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_INCLUDE_DIRS}"
            INTERFACE_LINK_OPTIONS "${PC_GStreamer_LDFLAGS_OTHER}"
        )
        _gst_strip_macos_absent_link_libs(PC_GStreamer_LINK_LIBRARIES)
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
    _gst_recover_split_pkgconfig_paths(PC_GStreamer_${_gst_PLUGIN}
        INCLUDE_DIRS CFLAGS_OTHER
        STATIC_INCLUDE_DIRS STATIC_CFLAGS_OTHER
        LIBRARY_DIRS LDFLAGS_OTHER
        STATIC_LIBRARY_DIRS STATIC_LDFLAGS_OTHER
    )

    set(GStreamer_${_gst_PLUGIN}_FOUND "${PC_GStreamer_${_gst_PLUGIN}_FOUND}" PARENT_SCOPE)
    if (NOT PC_GStreamer_${_gst_PLUGIN}_FOUND)
        if(GStreamer_DEBUG)
            message(STATUS "[GstFind] Component ${_gst_PLUGIN} (${_gst_PC_NAME}): NOT FOUND")
        endif()
        list(APPEND GStreamer_ABSENT_COMPONENTS ${_gst_PLUGIN})
        set(GStreamer_ABSENT_COMPONENTS "${GStreamer_ABSENT_COMPONENTS}"
            CACHE INTERNAL "GStreamer components probed and found absent")
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
        _gst_coalesce_existing_paths(${_pc}_STATIC_LIBRARY_DIRS)
        _gst_apply_frameworks(${_ldflags_var} GStreamer::${_gst_PLUGIN})
        _gst_resolve_and_link_libraries(GStreamer::${_gst_PLUGIN} INTERFACE "${${_pc}_STATIC_LIBRARIES}" "${${_pc}_STATIC_LIBRARY_DIRS}")
    else()
        _gst_coalesce_existing_paths(${_pc}_LIBRARY_DIRS)
        _gst_strip_macos_absent_link_libs(${_pc}_LINK_LIBRARIES)
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

if(PC_GStreamer_FOUND AND TARGET GStreamer::GStreamer)
    set(GStreamer_CORE_TARGET GStreamer::GStreamer)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GStreamer
    REQUIRED_VARS
        GStreamer_CORE_TARGET
        GStreamer_VERSION
        GStreamer_ROOT_DIR
    VERSION_VAR GStreamer_VERSION
    HANDLE_VERSION_RANGE
    HANDLE_COMPONENTS
)
