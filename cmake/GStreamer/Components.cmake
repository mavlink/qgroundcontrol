# GStreamer component names and plugin scanning helpers.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# gstreamer_platform_plugin_attrs(<ext_out> <prefix_out>)
# Sets the extension and file-name prefix of the platform's plugin libraries. Used by plugin discovery and post-install
# verify.
function(gstreamer_platform_plugin_attrs ext_out prefix_out)
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
endfunction()

# gstreamer_scan_plugin_basenames(<output_var> <plugin_path> [STATIC])
# Returns plugin basenames (e.g. "videoconvert", "x264") found in <plugin_path>,
# stripped of the platform's lib prefix and extension, or with STATIC of the static archives' libgst and .a. Empty
# string if path missing.
function(gstreamer_scan_plugin_basenames output_var plugin_path)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "STATIC" "" "")
    _qgc_gst_check_args(gstreamer_scan_plugin_basenames)
    if(NOT EXISTS "${plugin_path}")
        set(${output_var}
            ""
            PARENT_SCOPE
        )
        return()
    endif()
    if(ARG_STATIC)
        set(_prefix "libgst")
        set(_ext "a")
    else()
        gstreamer_platform_plugin_attrs(_ext _prefix)
    endif()
    # Underscore included: a hypothetical libgstvideo_foo.so must not truncate to "video".
    set(_re "^${_prefix}([a-zA-Z0-9_]+)")
    file(GLOB _files "${plugin_path}/${_prefix}*.${_ext}")
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

# The GStreamer libraries the build can request, in probe and link order. A component's pkg-config module and target
# follow from its CamelCase name: GlPrototypes is gstreamer-gl-prototypes-1.0 and GStreamer::api_gl_prototypes. The
# mandatory ones are always probed and fail configure when missing: the sources include their headers unconditionally.
# Gl/GlPrototypes are optional: a sysroot whose x11 .pc files are incomplete fails their pkg-config checks, and only the
# GL GPU path needs them.
set(GSTREAMER_COMPONENTS
    Base
    Gl
    GlPrototypes
    Rtsp
    Video
    App
    RtspServer
    Allocators
    Pbutils
    Tag
    Audio
)
set(GSTREAMER_MANDATORY_COMPONENTS Base Rtsp Video)

# gstreamer_component_names(<component> <api_out> <pc_out>)
# Sets <api_out> to the component's api_<snake_case> name and <pc_out> to its gstreamer-<kebab-case>-1.0 module.
function(gstreamer_component_names component api_out pc_out)
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" _snake "${component}")
    string(TOLOWER "${_snake}" _snake)
    string(REPLACE "_" "-" _module "${_snake}")
    set(${api_out}
        "api_${_snake}"
        PARENT_SCOPE
    )
    set(${pc_out}
        "gstreamer-${_module}-1.0"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_select_components(<out_var> <component>...)
# Sets <out_var> to the mandatory components and the given ones, in GSTREAMER_COMPONENTS order; fails on an unknown one.
function(gstreamer_select_components out_var)
    set(_unknown ${ARGN})
    list(REMOVE_ITEM _unknown ${GSTREAMER_COMPONENTS})
    if(_unknown)
        message(FATAL_ERROR "GStreamer: unknown components ${_unknown} (known: ${GSTREAMER_COMPONENTS})")
    endif()
    set(_selected "")
    foreach(_component IN LISTS GSTREAMER_COMPONENTS)
        if(_component IN_LIST GSTREAMER_MANDATORY_COMPONENTS OR _component IN_LIST ARGN)
            list(APPEND _selected ${_component})
        endif()
    endforeach()
    set(${out_var}
        "${_selected}"
        PARENT_SCOPE
    )
endfunction()
