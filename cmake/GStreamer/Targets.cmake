# SPDX-FileCopyrightText: 2024 L. E. Segovia <amy@centricular.com>
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Derived from FindGStreamer.cmake in the GStreamer monorepo
# (https://gitlab.freedesktop.org/gstreamer/gstreamer, subprojects/gstreamer/cmake/FindGStreamer.cmake),
# which the GStreamer SDKs install as share/cmake/FindGStreamer.cmake. QGC changes:
#   - a function called by Orchestrator.cmake instead of a find module: the caller resolves the SDK,
#     sets up the pkg-config environment (PkgConfig.cmake) and lists the APIs and plugins to create;
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

# _qgc_gst_create_component_target(<name> <pc_module> <static> [QUIET])
# Creates GStreamer::<name> from pkg-config module <pc_module>, when it exists, and links it into GStreamer::GStreamer.
function(_qgc_gst_create_component_target name pc_module static)
    if(TARGET GStreamer::${name})
        return()
    endif()

    set(_pc PC_GStreamer_${name})
    pkg_check_modules(${_pc} ${ARGN} "${pc_module}")
    if(NOT ${_pc}_FOUND)
        if(GStreamer_DEBUG)
            message(STATUS "[GstTargets] Component ${name} (${pc_module}): NOT FOUND")
        endif()
        return()
    endif()
    if(GStreamer_DEBUG)
        message(STATUS "[GstTargets] Component ${name} (${pc_module}): FOUND, STATIC_LIBS=${${_pc}_STATIC_LIBRARIES}")
    endif()
    _gst_recover_split_pkgconfig_paths(${_pc})

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
    target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::${name})
endfunction()

# _qgc_gst_create_core_targets(<minimum_version> <static> <version_var>)
# Creates GStreamer::GStreamer and GStreamer::deps from gstreamer-1.0 >= <minimum_version>.
function(_qgc_gst_create_core_targets minimum_version static version_var)
    pkg_check_modules(PC_GStreamer REQUIRED gstreamer-1.0>=${minimum_version})
    _gst_recover_split_pkgconfig_paths(PC_GStreamer)
    set(${version_var}
        "${PC_GStreamer_VERSION}"
        PARENT_SCOPE
    )
    if(GStreamer_DEBUG)
        message(STATUS "[GstTargets] PC_GStreamer_LIBRARIES = ${PC_GStreamer_LIBRARIES}")
        message(STATUS "[GstTargets] PC_GStreamer_STATIC_LIBRARIES = ${PC_GStreamer_STATIC_LIBRARIES}")
        message(STATUS "[GstTargets] PC_GStreamer_STATIC_LIBRARY_DIRS = ${PC_GStreamer_STATIC_LIBRARY_DIRS}")
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

# _qgc_gst_create_sdk_targets()
# Creates INTERFACE IMPORTED targets from the pkg-config view discovery set up for the SDK at GStreamer_ROOT_DIR:
#   GStreamer::GStreamer  gstreamer-1.0 >= QGC_GST_REQUESTED_VERSION, linking GStreamer::deps and every target below;
#   GStreamer::deps       the core's link dependencies;
#   GStreamer::<api>      gstreamer-<api>-1.0 for each GSTREAMER_APIS entry found (api_gl_prototypes ->
#                         gstreamer-gl-prototypes-1.0);
#   GStreamer::<plugin>   gst<plugin> for each GSTREAMER_PLUGINS entry found, static layouts only (shared layouts load
#                         plugins at runtime and never link them).
# Then applies the macOS framework overlay or builds the Android mobile target. Sets GStreamer_VERSION, on static
# layouts appends --static to PKG_CONFIG_ARGN and, on Android, sets GSTREAMER_STATIC_PLUGINS, all in the caller's scope.
# cmake-lint: disable=C0103,E1121
function(_qgc_gst_create_sdk_targets)
    set(_outputs GStreamer_VERSION)
    # Normal variable: the GStreamer subtree's later pkg-config probes (HwBuffers) need the same flags.
    if(GStreamer_USE_STATIC_LIBS AND NOT "--static" IN_LIST PKG_CONFIG_ARGN)
        list(APPEND PKG_CONFIG_ARGN "--static")
        list(APPEND _outputs PKG_CONFIG_ARGN)
    endif()
    if(GStreamer_USE_STATIC_LIBS)
        set(_static TRUE)
        set(_plugins ${GSTREAMER_PLUGINS})
    else()
        set(_static FALSE)
        set(_plugins "")
    endif()

    # The framework lookups below search the SDK's framework too; pkg-config is already locked to the SDK
    # (gstreamer_apply_pkgconfig_env).
    if(GStreamer_USE_FRAMEWORK)
        list(APPEND CMAKE_FRAMEWORK_PATH "${GSTREAMER_FRAMEWORK_PATH}")
    endif()

    # QGC_GST_REQUESTED_VERSION is the per-platform floor chosen by Orchestrator.cmake — Linux honors the distro
    # minimum (1.20), bundled-SDK platforms enforce ≥1.28 so a downgraded SDK fails configure loudly.
    _qgc_gst_create_core_targets("${QGC_GST_REQUESTED_VERSION}" ${_static} GStreamer_VERSION)
    foreach(_component IN LISTS GSTREAMER_COMPONENTS)
        gstreamer_component_names(${_component} _api _pc)
        if(NOT _api IN_LIST GSTREAMER_APIS)
            continue()
        endif()
        # A missing mandatory module fails configure with its own message (Orchestrator.cmake); the others are optional.
        set(_quiet QUIET)
        if(_component IN_LIST GSTREAMER_MANDATORY_COMPONENTS)
            set(_quiet "")
        endif()
        _qgc_gst_create_component_target(${_api} "${_pc}" ${_static} ${_quiet})
    endforeach()
    foreach(_plugin IN LISTS _plugins)
        _qgc_gst_create_component_target(${_plugin} "gst${_plugin}" TRUE QUIET)
    endforeach()

    # Apple framework overlay — defined in platform/MacOS.cmake; iOS uses its merged archive.
    if(MACOS AND GStreamer_USE_FRAMEWORK)
        _qgc_gst_apply_macos_framework_overlay()
    endif()

    # Android mobile-target setup: bundle plugins + optional fonts/CA assets into the
    # GStreamer::mobile target. iOS builds its targets from the merged archive (platform/IOS.cmake), so this is
    # Android-only.
    if(ANDROID)
        _qgc_gst_create_android_mobile_target()
        list(APPEND _outputs GSTREAMER_STATIC_PLUGINS)
    endif()
    return(PROPAGATE ${_outputs})
endfunction()
