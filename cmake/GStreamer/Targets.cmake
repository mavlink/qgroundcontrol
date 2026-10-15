# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Derived from FindGStreamer.cmake in the GStreamer monorepo
# (https://gitlab.freedesktop.org/gstreamer/gstreamer, subprojects/gstreamer/cmake/FindGStreamer.cmake),
# which the GStreamer SDKs install as share/cmake/FindGStreamer.cmake. QGC changes:
#   - a function called by Orchestrator.cmake instead of a find module: the caller resolves the SDK,
#     sets up the pkg-config environment (PkgConfig.cmake) and passes the APIs and plugins to create;
#   - library resolution and linking live in Link.cmake (_gst_resolve_and_link_libraries);
#   - GStreamer_EXTRA_DEPS is not supported;
#   - plugin targets are only created for static layouts, the only ones that link them.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Link.cmake")

# _qgc_gst_existing_dirs(<list_var>)
# Drops the directories in <list_var> that do not exist, warning about each one.
function(_qgc_gst_existing_dirs list_var)
    _gst_coalesce_existing_paths(${list_var})
    set(_existing "")
    foreach(_dir IN LISTS ${list_var})
        if(EXISTS "${_dir}")
            list(APPEND _existing "${_dir}")
        else()
            message(WARNING "Skipping missing include folder ${_dir}.")
        endif()
    endforeach()
    set(${list_var}
        "${_existing}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_set_link_options(<target> <ldflag>...)
# Sets INTERFACE_LINK_OPTIONS from pkg-config link flags. On Apple, "-framework <name>" pairs are
# linked as the framework found by find_library instead.
function(_qgc_gst_set_link_options target)
    if(NOT APPLE)
        set_target_properties(${target} PROPERTIES INTERFACE_LINK_OPTIONS "${ARGN}")
        return()
    endif()
    set(_options "")
    set(_framework_next FALSE)
    foreach(_arg IN LISTS ARGN)
        if(_framework_next)
            set(_framework_next FALSE)
            find_library(GStreamer_FW_${_arg}_LIB ${_arg} REQUIRED)
            target_link_libraries(${target} INTERFACE "${GStreamer_FW_${_arg}_LIB}")
        elseif(_arg STREQUAL "-framework")
            set(_framework_next TRUE)
        else()
            list(APPEND _options "${_arg}")
        endif()
    endforeach()
    if(_framework_next)
        message(WARNING "GStreamer: trailing -framework with no name in ${ARGN}")
    endif()
    set_target_properties(${target} PROPERTIES INTERFACE_LINK_OPTIONS "${_options}")
endfunction()

# _qgc_gst_create_component_target(<name> <pc_module> <static> <found_var> [QUIET])
# Creates GStreamer::<name> from pkg-config module <pc_module>; <found_var> tells whether it exists.
function(_qgc_gst_create_component_target name pc_module static found_var)
    if(TARGET GStreamer::${name})
        set(${found_var}
            TRUE
            PARENT_SCOPE
        )
        return()
    endif()

    set(_pc PC_GStreamer_${name})
    pkg_check_modules(${_pc} ${ARGN} "${pc_module}")
    set(${found_var}
        "${${_pc}_FOUND}"
        PARENT_SCOPE
    )
    if(NOT ${_pc}_FOUND)
        if(GStreamer_DEBUG)
            message(STATUS "[GstTargets] Component ${name} (${pc_module}): NOT FOUND")
        endif()
        return()
    endif()
    if(GStreamer_DEBUG)
        message(STATUS "[GstTargets] Component ${name} (${pc_module}): FOUND, STATIC_LIBS=${${_pc}_STATIC_LIBRARIES}")
    endif()
    _gst_recover_split_pkgconfig_paths(
        ${_pc}
        INCLUDE_DIRS
        CFLAGS_OTHER
        STATIC_INCLUDE_DIRS
        STATIC_CFLAGS_OTHER
        LIBRARY_DIRS
        LDFLAGS_OTHER
        STATIC_LIBRARY_DIRS
        STATIC_LDFLAGS_OTHER
    )

    if(static)
        set(_kind STATIC_)
    else()
        set(_kind "")
    endif()
    add_library(GStreamer::${name} INTERFACE IMPORTED GLOBAL)
    _qgc_gst_existing_dirs(${_pc}_${_kind}INCLUDE_DIRS)
    set_target_properties(GStreamer::${name} PROPERTIES INTERFACE_COMPILE_OPTIONS "${${_pc}_${_kind}CFLAGS_OTHER}")
    if(${_pc}_${_kind}INCLUDE_DIRS)
        set_target_properties(GStreamer::${name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                            "${${_pc}_${_kind}INCLUDE_DIRS}"
        )
    endif()

    if(static)
        _gst_coalesce_existing_paths(${_pc}_STATIC_LIBRARY_DIRS)
        _qgc_gst_set_link_options(GStreamer::${name} ${${_pc}_STATIC_LDFLAGS_OTHER})
        _gst_resolve_and_link_libraries(GStreamer::${name} INTERFACE "${${_pc}_STATIC_LIBRARIES}"
                                        "${${_pc}_STATIC_LIBRARY_DIRS}"
        )
    else()
        _gst_strip_macos_absent_link_libs(${_pc}_LINK_LIBRARIES)
        set_target_properties(GStreamer::${name} PROPERTIES INTERFACE_LINK_OPTIONS "${${_pc}_LDFLAGS_OTHER}"
                                                            INTERFACE_LINK_LIBRARIES "${${_pc}_LINK_LIBRARIES}"
        )
    endif()
endfunction()

# _qgc_gst_create_core_targets(<minimum_version> <static> <version_var>)
# Creates GStreamer::GStreamer and GStreamer::deps from gstreamer-1.0 >= <minimum_version>.
function(_qgc_gst_create_core_targets minimum_version static version_var)
    pkg_check_modules(PC_GStreamer REQUIRED gstreamer-1.0>=${minimum_version})
    _gst_recover_split_pkgconfig_paths(
        PC_GStreamer
        INCLUDE_DIRS
        CFLAGS_OTHER
        STATIC_INCLUDE_DIRS
        STATIC_CFLAGS_OTHER
        LIBRARY_DIRS
        LDFLAGS_OTHER
        STATIC_LIBRARY_DIRS
        STATIC_LDFLAGS_OTHER
    )
    set(${version_var}
        "${PC_GStreamer_VERSION}"
        PARENT_SCOPE
    )
    if(GStreamer_DEBUG)
        message(STATUS "[GstTargets] PC_GStreamer_LIBRARIES = ${PC_GStreamer_LIBRARIES}")
        message(STATUS "[GstTargets] PC_GStreamer_STATIC_LIBRARIES = ${PC_GStreamer_STATIC_LIBRARIES}")
        message(STATUS "[GstTargets] PC_GStreamer_STATIC_LIBRARY_DIRS = ${PC_GStreamer_STATIC_LIBRARY_DIRS}")
    endif()
    if(TARGET GStreamer::GStreamer)
        return()
    endif()

    add_library(GStreamer::GStreamer INTERFACE IMPORTED GLOBAL)
    add_library(GStreamer::deps INTERFACE IMPORTED GLOBAL)
    if(static)
        _qgc_gst_existing_dirs(PC_GStreamer_STATIC_INCLUDE_DIRS)
        _gst_coalesce_existing_paths(PC_GStreamer_STATIC_LIBRARY_DIRS)
        set_target_properties(GStreamer::GStreamer PROPERTIES INTERFACE_COMPILE_OPTIONS
                                                              "${PC_GStreamer_STATIC_CFLAGS_OTHER}"
        )
        if(PC_GStreamer_STATIC_INCLUDE_DIRS)
            set_target_properties(GStreamer::GStreamer PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                                                  "${PC_GStreamer_STATIC_INCLUDE_DIRS}"
            )
        endif()
        _qgc_gst_set_link_options(GStreamer::GStreamer ${PC_GStreamer_STATIC_LDFLAGS_OTHER})
        _gst_resolve_and_link_libraries(GStreamer::GStreamer INTERFACE "${PC_GStreamer_LIBRARIES}"
                                        "${PC_GStreamer_STATIC_LIBRARY_DIRS}"
        )
        _gst_resolve_and_link_libraries(GStreamer::deps INTERFACE "${PC_GStreamer_STATIC_LIBRARIES}"
                                        "${PC_GStreamer_STATIC_LIBRARY_DIRS}" HIDE
        )
    else()
        _qgc_gst_existing_dirs(PC_GStreamer_INCLUDE_DIRS)
        set_target_properties(
            GStreamer::GStreamer
            PROPERTIES INTERFACE_COMPILE_OPTIONS "${PC_GStreamer_CFLAGS_OTHER}"
                       INTERFACE_INCLUDE_DIRECTORIES "${PC_GStreamer_INCLUDE_DIRS}"
                       INTERFACE_LINK_OPTIONS "${PC_GStreamer_LDFLAGS_OTHER}"
        )
        _gst_strip_macos_absent_link_libs(PC_GStreamer_LINK_LIBRARIES)
        set_target_properties(GStreamer::deps PROPERTIES INTERFACE_LINK_LIBRARIES "${PC_GStreamer_LINK_LIBRARIES}")
    endif()
    target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::deps)
endfunction()

# qgc_gst_create_pkgconfig_targets(ROOT <sdk_root> VERSION <minimum> APIS <api>... [PLUGINS <plugin>...]
#                                  [STATIC] [FRAMEWORK_PATH <dir>])
# Creates INTERFACE IMPORTED targets from pkg-config, which must already be set up for the SDK
# (<sdk_root> and FRAMEWORK_PATH are only added to the search paths of this call):
#   GStreamer::GStreamer  gstreamer-1.0 >= <minimum>, linking GStreamer::deps and every target below;
#   GStreamer::deps       the core's link dependencies;
#   GStreamer::<api>      gstreamer-<api>-1.0 for each api_<api> found (api_gl_prototypes ->
#                         gstreamer-gl-prototypes-1.0);
#   GStreamer::<plugin>   gst<plugin> for each plugin found, STATIC only (shared layouts load plugins at
#                         runtime and never link them).
# Sets GStreamer_VERSION and GStreamer_<api|plugin>_FOUND for each module probed in the caller's scope.
function(qgc_gst_create_pkgconfig_targets)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "STATIC" "ROOT;VERSION;FRAMEWORK_PATH" "APIS;PLUGINS")
    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "qgc_gst_create_pkgconfig_targets: missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "qgc_gst_create_pkgconfig_targets: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_ROOT OR NOT EXISTS "${ARG_ROOT}")
        message(FATAL_ERROR "qgc_gst_create_pkgconfig_targets: ROOT must be an existing directory "
                            "(current value: '${ARG_ROOT}')"
        )
    endif()
    if(NOT ARG_VERSION)
        message(FATAL_ERROR "qgc_gst_create_pkgconfig_targets: VERSION is required")
    endif()
    if(ARG_STATIC)
        set(_static TRUE)
        set(_plugins ${ARG_PLUGINS})
    else()
        set(_static FALSE)
        set(_plugins "")
    endif()

    list(PREPEND CMAKE_PREFIX_PATH "${ARG_ROOT}")
    if(ARG_FRAMEWORK_PATH)
        list(APPEND CMAKE_FRAMEWORK_PATH "${ARG_FRAMEWORK_PATH}")
    endif()

    find_package(PkgConfig REQUIRED QUIET)
    _qgc_gst_create_core_targets("${ARG_VERSION}" ${_static} _version)
    set(GStreamer_VERSION
        "${_version}"
        PARENT_SCOPE
    )

    foreach(_api IN LISTS ARG_APIS)
        string(REGEX REPLACE "^api_" "" _pc_module "${_api}")
        string(REPLACE "_" "-" _pc_module "${_pc_module}")
        _qgc_gst_create_component_target(${_api} "gstreamer-${_pc_module}-1.0" ${_static} _found)
        set(GStreamer_${_api}_FOUND
            "${_found}"
            PARENT_SCOPE
        )
        if(_found)
            target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::${_api})
        endif()
    endforeach()
    foreach(_plugin IN LISTS _plugins)
        _qgc_gst_create_component_target(${_plugin} "gst${_plugin}" TRUE _found QUIET)
        set(GStreamer_${_plugin}_FOUND
            "${_found}"
            PARENT_SCOPE
        )
        if(_found)
            target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::${_plugin})
        endif()
    endforeach()
endfunction()
