# Component registry, api/pc-name mapping, and plugin scanning helpers.
# GSTREAMER_COMPONENT_REGISTRY below is the single source of truth for the
# component → api_name → pc_name → mandatory mapping.

include_guard(GLOBAL)

# Single source of truth for platform plugin shared-library naming triple
# (extension, prefix, glob). Used by plugin discovery and post-install verify.
function(gstreamer_platform_plugin_attrs EXT_OUT PREFIX_OUT GLOB_OUT)
    if(WIN32)
        set(_ext "dll")
        set(_prefix "gst")
    elseif(APPLE)
        set(_ext "dylib")
        set(_prefix "libgst")
    else()
        set(_ext "so")
        set(_prefix "libgst")
    endif()
    set(${EXT_OUT}    "${_ext}"             PARENT_SCOPE)
    set(${PREFIX_OUT} "${_prefix}"          PARENT_SCOPE)
    set(${GLOB_OUT}   "${_prefix}*.${_ext}" PARENT_SCOPE)
endfunction()

# Returns plugin basenames (e.g. "videoconvert", "x264") found in PLUGIN_PATH,
# stripped of the platform's lib prefix and extension. Empty string if path missing.
function(gstreamer_scan_plugin_basenames OUTPUT_VAR PLUGIN_PATH)
    if(NOT EXISTS "${PLUGIN_PATH}")
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()
    gstreamer_platform_plugin_attrs(_ext _prefix _glob)
    # Underscore included: a hypothetical libgstvideo_foo.so must not truncate to "video".
    set(_re "^${_prefix}([a-zA-Z0-9_]+)")
    file(GLOB _files "${PLUGIN_PATH}/${_glob}")
    set(_names "")
    foreach(_p IN LISTS _files)
        get_filename_component(_n "${_p}" NAME)
        if(_n MATCHES "${_re}")
            list(APPEND _names "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _names)
    set(${OUTPUT_VAR} "${_names}" PARENT_SCOPE)
endfunction()

# Component registry — SINGLE source of truth for the
# (component_name, api_name, pc_name, mandatory) tuple.
#
#   Format per entry: "ComponentName:api_name:pc_name:mandatory"
#     ComponentName  - user-facing CamelCase component (Core, Base, GlPrototypes, …)
#     api_name       - snake_case CMake/imported-target stem (api_base, api_gl_prototypes, …);
#                      empty for components with no .pc file (Core)
#     pc_name        - pkg-config module name (gstreamer-base-1.0); empty when no .pc file
#     mandatory      - "1" if always present in any GStreamer install, "0" otherwise
#
# Mandatory entries seed gstreamer_build_apis_and_deps and the always-FOUND
# iteration in Orchestrator.cmake. Adding a new always-present API requires
# only an entry here.
set(GSTREAMER_COMPONENT_REGISTRY
    "Core:::1"                                                   # umbrella; gstreamer-1.0 is queried directly
    "Base:api_base:gstreamer-base-1.0:1"
    "Gl:api_gl:gstreamer-gl-1.0:1"
    "GlPrototypes:api_gl_prototypes:gstreamer-gl-prototypes-1.0:1"
    "Rtsp:api_rtsp:gstreamer-rtsp-1.0:1"
    "Video:api_video:gstreamer-video-1.0:1"
    "App:api_app:gstreamer-app-1.0:0"
    "Allocators:api_allocators:gstreamer-allocators-1.0:0"
    "Pbutils:api_pbutils:gstreamer-pbutils-1.0:0"
    "Tag:api_tag:gstreamer-tag-1.0:0"
    "Audio:api_audio:gstreamer-audio-1.0:0"
)

# Internal: split a registry entry "Component:api:pc:mandatory" into 4 OUT vars.
function(_gstreamer_registry_split ENTRY OUT_NAME OUT_API OUT_PC OUT_MAND)
    string(REPLACE ":" ";" _parts "${ENTRY}")
    list(LENGTH _parts _n)
    if(NOT _n EQUAL 4)
        message(FATAL_ERROR "GSTREAMER_COMPONENT_REGISTRY: malformed entry '${ENTRY}' (need exactly 4 colon-separated fields, got ${_n})")
    endif()
    list(GET _parts 0 _name)
    list(GET _parts 1 _api)
    list(GET _parts 2 _pc)
    list(GET _parts 3 _mand)
    string(STRIP "${_mand}" _mand)
    set(${OUT_NAME} "${_name}" PARENT_SCOPE)
    set(${OUT_API}  "${_api}"  PARENT_SCOPE)
    set(${OUT_PC}   "${_pc}"   PARENT_SCOPE)
    set(${OUT_MAND} "${_mand}" PARENT_SCOPE)
endfunction()

# gstreamer_resolve_component(<query> <out_name> <out_api> <out_pc> <out_mandatory>)
# Looks up the registry by either ComponentName or api_name. Returns empty
# strings for all OUT vars when the query doesn't match any entry, with a
# fallback for unknown api_foo queries (out_pc set via the api_foo →
# gstreamer-foo-1.0 convention; out_name/out_mandatory left empty).
function(gstreamer_resolve_component QUERY OUT_NAME OUT_API OUT_PC OUT_MANDATORY)
    foreach(_entry IN LISTS GSTREAMER_COMPONENT_REGISTRY)
        _gstreamer_registry_split("${_entry}" _name _api _pc _mand)
        if(QUERY STREQUAL _name OR (_api AND QUERY STREQUAL _api))
            set(${OUT_NAME}      "${_name}" PARENT_SCOPE)
            set(${OUT_API}       "${_api}"  PARENT_SCOPE)
            set(${OUT_PC}        "${_pc}"   PARENT_SCOPE)
            set(${OUT_MANDATORY} "${_mand}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
    # Fallback for unknown api_foo_bar queries: gstreamer-foo-bar-1.0.
    if(QUERY MATCHES "^api_(.+)$")
        string(REPLACE "_" "-" _pc "${CMAKE_MATCH_1}")
        set(${OUT_NAME}      ""                       PARENT_SCOPE)
        set(${OUT_API}       "${QUERY}"               PARENT_SCOPE)
        set(${OUT_PC}        "gstreamer-${_pc}-1.0"   PARENT_SCOPE)
        set(${OUT_MANDATORY} "0"                      PARENT_SCOPE)
        return()
    endif()
    set(${OUT_NAME}      "" PARENT_SCOPE)
    set(${OUT_API}       "" PARENT_SCOPE)
    set(${OUT_PC}        "" PARENT_SCOPE)
    set(${OUT_MANDATORY} "" PARENT_SCOPE)
endfunction()

# gstreamer_mandatory_components(<out_names_list> <out_apis_list>)
# Returns the registry's always-present component names and api_ names. Used
# to drive the FOUND-loop in Orchestrator.cmake and the api-seed in
# gstreamer_build_apis_and_deps so neither has to repeat the list.
function(gstreamer_mandatory_components OUT_NAMES OUT_APIS)
    set(_names "")
    set(_apis "")
    foreach(_entry IN LISTS GSTREAMER_COMPONENT_REGISTRY)
        _gstreamer_registry_split("${_entry}" _name _api _pc _mand)
        if(_mand STREQUAL "1")
            list(APPEND _names "${_name}")
            if(_api)
                list(APPEND _apis "${_api}")
            endif()
        endif()
    endforeach()
    set(${OUT_NAMES} "${_names}" PARENT_SCOPE)
    set(${OUT_APIS}  "${_apis}"  PARENT_SCOPE)
endfunction()

# gstreamer_component_to_api(<component> <out_var>)
# CamelCase component -> api_snake_case (fallback for unregistered components).
function(gstreamer_component_to_api COMPONENT OUT_VAR)
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" _snake "${COMPONENT}")
    string(TOLOWER "${_snake}" _snake)
    set(${OUT_VAR} "api_${_snake}" PARENT_SCOPE)
endfunction()

# gstreamer_build_apis_and_deps(<apis_out> <deps_out> <component_list>)
# Builds GSTREAMER_APIS and GSTREAMER_EXTRA_DEPS from a component list plus
# optional platform extras. The seed of always-present APIs comes from the
# registry's mandatory rows (no hardcoded list).
function(gstreamer_build_apis_and_deps APIS_OUT DEPS_OUT)
    # Seed from registry-mandatory rows (single source of truth).
    gstreamer_mandatory_components(_seed_names _apis)

    # Add extra apis from caller's component list — resolve via registry first
    # to handle CamelCase → api_snake mapping for non-mandatory components.
    foreach(_comp IN LISTS ARGN)
        if(_comp STREQUAL "Core")
            continue()
        endif()
        gstreamer_resolve_component("${_comp}" _name _api _pc _mand)
        if(NOT _api)
            # Unknown component name: derive api via CamelCase → snake_case.
            gstreamer_component_to_api("${_comp}" _api)
        endif()
        if(NOT _api IN_LIST _apis)
            list(APPEND _apis "${_api}")
        endif()
    endforeach()

    # Derive pkg-config deps from registry.
    set(_deps)
    foreach(_api IN LISTS _apis)
        gstreamer_resolve_component("${_api}" _ _ _pc _)
        if(_pc)
            list(APPEND _deps "${_pc}")
        endif()
    endforeach()

    # Platform extra deps — kept here as the single registration point.
    if(WIN32)
        list(APPEND _deps graphene-1.0)
    endif()
    if(ANDROID OR IOS)
        list(APPEND _deps gio-2.0)
    endif()
    if(ANDROID)
        list(APPEND _deps gmodule-2.0 zlib)
    endif()

    set(${APIS_OUT} "${_apis}" PARENT_SCOPE)
    set(${DEPS_OUT} "${_deps}" PARENT_SCOPE)
endfunction()
