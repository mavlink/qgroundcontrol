# Component registry, api/pc-name mapping, and plugin scanning helpers.
# GSTREAMER_COMPONENT_REGISTRY below is the single source of truth for the
# component → api_name → pc_name → mandatory mapping.

include_guard(GLOBAL)

# Single source of truth for platform plugin shared-library naming triple
# (extension, prefix, glob). Used by plugin discovery and post-install verify.
function(gstreamer_platform_plugin_attrs ext_out prefix_out glob_out)
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
    set(${ext_out}
        "${_ext}"
        PARENT_SCOPE
    )
    set(${prefix_out}
        "${_prefix}"
        PARENT_SCOPE
    )
    set(${glob_out}
        "${_prefix}*.${_ext}"
        PARENT_SCOPE
    )
endfunction()

# Returns plugin basenames (e.g. "videoconvert", "x264") found in <plugin_path>,
# stripped of the platform's lib prefix and extension. Empty string if path missing.
function(gstreamer_scan_plugin_basenames output_var plugin_path)
    if(NOT EXISTS "${plugin_path}")
        set(${output_var}
            ""
            PARENT_SCOPE
        )
        return()
    endif()
    gstreamer_platform_plugin_attrs(_ext _prefix _glob)
    # Underscore included: a hypothetical libgstvideo_foo.so must not truncate to "video".
    set(_re "^${_prefix}([a-zA-Z0-9_]+)")
    file(GLOB _files "${plugin_path}/${_glob}")
    set(_names "")
    foreach(_p IN LISTS _files)
        get_filename_component(_n "${_p}" NAME)
        if(_n MATCHES "${_re}")
            list(APPEND _names "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _names)
    set(${output_var}
        "${_names}"
        PARENT_SCOPE
    )
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
# Mandatory entries seed gstreamer_build_apis and the mandatory-module check
# in Orchestrator.cmake. Adding a new always-present API requires only an
# entry here. Gl/GlPrototypes are optional: a sysroot whose x11 .pc files are
# incomplete fails their pkg-config checks, and only the GL GPU path needs them.
set(GSTREAMER_COMPONENT_REGISTRY
    "Core:::1" # umbrella; gstreamer-1.0 is queried directly
    "Base:api_base:gstreamer-base-1.0:1"
    "Gl:api_gl:gstreamer-gl-1.0:0"
    "GlPrototypes:api_gl_prototypes:gstreamer-gl-prototypes-1.0:0"
    "Rtsp:api_rtsp:gstreamer-rtsp-1.0:1"
    "Video:api_video:gstreamer-video-1.0:1"
    "App:api_app:gstreamer-app-1.0:0"
    "RtspServer:api_rtsp_server:gstreamer-rtsp-server-1.0:0"
    "Allocators:api_allocators:gstreamer-allocators-1.0:0"
    "Pbutils:api_pbutils:gstreamer-pbutils-1.0:0"
    "Tag:api_tag:gstreamer-tag-1.0:0"
    "Audio:api_audio:gstreamer-audio-1.0:0"
)

# Internal: split a registry entry "Component:api:pc:mandatory" into 4 OUT vars.
function(_qgc_gst_registry_split entry out_name out_api out_pc out_mand)
    string(REPLACE ":" ";" _parts "${entry}")
    list(LENGTH _parts _n)
    if(NOT _n EQUAL 4)
        message(FATAL_ERROR "GSTREAMER_COMPONENT_REGISTRY: malformed entry '${entry}' "
                            "(need exactly 4 colon-separated fields, got ${_n})"
        )
    endif()
    list(GET _parts 0 _name)
    list(GET _parts 1 _api)
    list(GET _parts 2 _pc)
    list(GET _parts 3 _mand)
    string(STRIP "${_mand}" _mand)
    set(${out_name}
        "${_name}"
        PARENT_SCOPE
    )
    set(${out_api}
        "${_api}"
        PARENT_SCOPE
    )
    set(${out_pc}
        "${_pc}"
        PARENT_SCOPE
    )
    set(${out_mand}
        "${_mand}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_resolve_component(<query> <out_name> <out_api> <out_pc> <out_mandatory>)
# Looks up the registry by either ComponentName or api_name. Returns empty
# strings for all OUT vars when the query doesn't match any entry, with a
# fallback for unknown api_foo queries (out_pc set via the api_foo →
# gstreamer-foo-1.0 convention; out_name/out_mandatory left empty).
function(gstreamer_resolve_component query out_name out_api out_pc out_mandatory)
    foreach(_entry IN LISTS GSTREAMER_COMPONENT_REGISTRY)
        _qgc_gst_registry_split("${_entry}" _name _api _pc _mand)
        if(query STREQUAL _name OR (_api AND query STREQUAL _api))
            set(${out_name}
                "${_name}"
                PARENT_SCOPE
            )
            set(${out_api}
                "${_api}"
                PARENT_SCOPE
            )
            set(${out_pc}
                "${_pc}"
                PARENT_SCOPE
            )
            set(${out_mandatory}
                "${_mand}"
                PARENT_SCOPE
            )
            return()
        endif()
    endforeach()
    # Fallback for unknown api_foo_bar queries: gstreamer-foo-bar-1.0.
    if(query MATCHES "^api_(.+)$")
        string(REPLACE "_" "-" _pc "${CMAKE_MATCH_1}")
        set(${out_name}
            ""
            PARENT_SCOPE
        )
        set(${out_api}
            "${query}"
            PARENT_SCOPE
        )
        set(${out_pc}
            "gstreamer-${_pc}-1.0"
            PARENT_SCOPE
        )
        set(${out_mandatory}
            "0"
            PARENT_SCOPE
        )
        return()
    endif()
    set(${out_name}
        ""
        PARENT_SCOPE
    )
    set(${out_api}
        ""
        PARENT_SCOPE
    )
    set(${out_pc}
        ""
        PARENT_SCOPE
    )
    set(${out_mandatory}
        ""
        PARENT_SCOPE
    )
endfunction()

# gstreamer_mandatory_components(<out_names_list> <out_apis_list>)
# Returns the registry's always-present component names and api_ names. Used
# by the mandatory-module check in Orchestrator.cmake and the api-seed in
# gstreamer_build_apis so neither has to repeat the list.
function(gstreamer_mandatory_components out_names out_apis)
    set(_names "")
    set(_apis "")
    foreach(_entry IN LISTS GSTREAMER_COMPONENT_REGISTRY)
        _qgc_gst_registry_split("${_entry}" _name _api _pc _mand)
        if(_mand STREQUAL "1")
            list(APPEND _names "${_name}")
            if(_api)
                list(APPEND _apis "${_api}")
            endif()
        endif()
    endforeach()
    set(${out_names}
        "${_names}"
        PARENT_SCOPE
    )
    set(${out_apis}
        "${_apis}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_component_to_api(<component> <out_var>)
# CamelCase component -> api_snake_case (fallback for unregistered components).
function(gstreamer_component_to_api component out_var)
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" _snake "${component}")
    string(TOLOWER "${_snake}" _snake)
    set(${out_var}
        "api_${_snake}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_build_apis(<apis_out> <component>...)
# Sets <apis_out> to the api_ names of the registry's mandatory rows plus those of the given components, in registry
# order (unregistered components last), so the probe and link order doesn't depend on how a caller lists them.
function(gstreamer_build_apis apis_out)
    # Resolve via the registry first to map CamelCase components to their api_ names.
    set(_requested "")
    foreach(_comp IN LISTS ARGN)
        if(_comp STREQUAL "Core")
            continue()
        endif()
        gstreamer_resolve_component("${_comp}" _name _api _pc _mand)
        if(NOT _api)
            # Unknown component name: derive api via CamelCase → snake_case.
            gstreamer_component_to_api("${_comp}" _api)
        endif()
        list(APPEND _requested "${_api}")
    endforeach()

    set(_apis "")
    foreach(_entry IN LISTS GSTREAMER_COMPONENT_REGISTRY)
        _qgc_gst_registry_split("${_entry}" _name _api _pc _mand)
        if(_api AND (_mand STREQUAL "1" OR _api IN_LIST _requested))
            list(APPEND _apis "${_api}")
        endif()
    endforeach()
    foreach(_api IN LISTS _requested)
        if(NOT _api IN_LIST _apis)
            list(APPEND _apis "${_api}")
        endif()
    endforeach()

    set(${apis_out}
        "${_apis}"
        PARENT_SCOPE
    )
endfunction()
