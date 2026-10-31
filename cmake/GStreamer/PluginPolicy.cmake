# GStreamer plugins: the policy (alternate groups, bundle-required and optional plugins, the GPL denylist), the
# allowlist of build-config.json, finding the plugin files an SDK ships, and the configure-time and install-time checks
# Orchestrator.cmake and Install.cmake run with them.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/../modules/BuildConfigReader.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/Platform.cmake")

# Plugin alternate groups — within each group, only one member typically ships
# per SDK (e.g. videoconvertscale is the 1.22 fusion of videoconvert+videoscale).
# Format: each entry is a "|"-separated alternate set; member sub-groups are
# "+"-separated for AND-satisfaction. Example: "videoconvertscale|videoconvert+videoscale"
# means satisfied if videoconvertscale is present OR if both videoconvert AND
# videoscale are present. Plugin basenames must not contain '|' or '+' — those
# are the group/AND delimiters and are not escapable.
set(GSTREAMER_PLUGIN_ALTERNATES "videoconvertscale|videoconvert+videoscale")

# Structural plugins every bundled GStreamer must ship: a mobile configure and the
# desktop install verifier fail without one. At runtime GStreamer.cc checks the
# elements per feature instead, and fails initialization only without the display
# core.
# Codec implementations (openh264/libav) are intentionally excluded: distro
# bundles may omit one while shipping another usable decode path.
# opengl supplies glupload/glcolorconvert (GL zero-copy video path);
# isomp4 (mp4mux/qtmux) and matroska (matroskamux) are the recording muxers
# (RecordingBranch.cc).
set(GSTREAMER_BUNDLE_REQUIRED_PLUGINS
    coreelements
    isomp4
    matroska
    opengl
    playback
    rtsp
    rtp
    rtpmanager
    tcp
    udp
    videoparsersbad
    videoconvertscale
)

# Hardware-specific and alternative decoder plugins: decoding uses whichever of them a host or SDK provides, so the
# Orchestrator only lists missing ones in a status line. The other allowlisted plugins are warned about.
set(GSTREAMER_OPTIONAL_PLUGINS
    androidmedia
    applemedia
    d3d
    d3d11
    d3d12
    dav1d
    libav
    nvcodec
    openh264
    qsv
    va
    vpx
    vulkan
)

# Encode-side plugins needed only by the Debug-only MockLink test video stream server (MockVideoStreamServer), which
# encodes with openh264enc (allowlisted). Statically registered on Android and iOS Debug builds only: desktop builds
# load whatever the SDK or system ships, and release builds must not carry them.
set(GSTREAMER_MOCK_SERVER_DEBUG_PLUGINS mpegtsmux videotestsrc)

# GPL-licensed plugins and libraries of the official SDKs (cerbero builds them with its gpl and ugly options) that QGC
# must never bundle: its GStreamer license notice (Notice.cmake) covers an LGPL GStreamer. The desktop installers fail
# an install that would ship one (Install.cmake).
set(GSTREAMER_GPL_DENYLIST
    a52
    a52dec
    cdio
    cdio_cdda
    cdio_paranoia
    dvdnav
    dvdread
    faad
    mpeg2
    mpeg2convert
    mpeg2dec
    mpeg2enc
    mpeg2encpp
    mplex
    mplex2
    resindvd
    sid
    sidplay
    x264
    x265
)

# qgc_gst_gpl_files(<out_var> <file>...)
# Sets <out_var> to the <file>s whose name is a GSTREAMER_GPL_DENYLIST library or plugin: the name, after an optional
# lib and gst prefix, followed by a version, extension or nothing (libx264.so.164, x264-164.dll, libgstx264.dylib).
function(qgc_gst_gpl_files out_var)
    list(JOIN GSTREAMER_GPL_DENYLIST "|" _names)
    set(_gpl "")
    foreach(_file IN LISTS ARGN)
        cmake_path(GET _file FILENAME _name)
        if(_name MATCHES "^(lib)?(gst)?(${_names})([-_.]|$)")
            list(APPEND _gpl "${_file}")
        endif()
    endforeach()
    set(${out_var}
        "${_gpl}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_platform_plugin_attrs(<ext_out> <prefix_out>)
# Sets the extension and file-name prefix of the platform's plugin libraries. Used by plugin discovery and post-install
# verify.
function(qgc_gst_platform_plugin_attrs ext_out prefix_out)
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

# qgc_gst_scan_plugin_basenames(<output_var> <plugin_path> [STATIC])
# Returns plugin basenames (e.g. "videoconvert", "x264") found in <plugin_path>,
# stripped of the platform's lib prefix and extension, or with STATIC of the static archives' libgst and .a. Empty
# string if path missing.
function(qgc_gst_scan_plugin_basenames output_var plugin_path)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "STATIC" "" "")
    _qgc_gst_check_args(qgc_gst_scan_plugin_basenames)
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
        qgc_gst_platform_plugin_attrs(_ext _prefix)
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

# qgc_gst_plugins_for(<key> <out_var>)
# Sets <out_var> to the plugin list of build-config.json for <key> ("android", "apple", "windows", "linux" or ""):
# gstreamer.plugins.common plus gstreamer.plugins.<key>.
function(qgc_gst_plugins_for key out_var)
    qgc_config_read(_plugins LIST gstreamer plugins common)
    if(key)
        qgc_config_read(_extra LIST OPTIONAL gstreamer plugins ${key})
        list(APPEND _plugins ${_extra})
    endif()
    set(${out_var}
        "${_plugins}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_missing_required(REQUIRED <plugin>... AVAILABLE <plugin>... OUT_VAR <var>)
# Sets <var> to the REQUIRED plugins, in order, that are not AVAILABLE. A member of a GSTREAMER_PLUGIN_ALTERNATES group
# is also satisfied by any alternate of its group whose plugins are all AVAILABLE, so the install verifier accepts
# videoconvert+videoscale (1.20) in place of the fused videoconvertscale (1.22+).
function(qgc_gst_missing_required)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "OUT_VAR" "REQUIRED;AVAILABLE")
    _qgc_gst_check_args(qgc_gst_missing_required OUT_VAR ALLOW_EMPTY REQUIRED AVAILABLE)
    set(_missing "")
    foreach(_plugin IN LISTS ARG_REQUIRED)
        # The plugin sets, each "+"-joined, that satisfy _plugin: itself, then the alternates of its group.
        set(_alternates "${_plugin}")
        foreach(_group IN LISTS GSTREAMER_PLUGIN_ALTERNATES)
            string(REGEX REPLACE "[|+]" ";" _group_plugins "${_group}")
            if(_plugin IN_LIST _group_plugins)
                string(REPLACE "|" ";" _group_alternates "${_group}")
                list(APPEND _alternates ${_group_alternates})
                break()
            endif()
        endforeach()
        set(_satisfied FALSE)
        foreach(_alternate IN LISTS _alternates)
            string(REPLACE "+" ";" _alternate_plugins "${_alternate}")
            set(_satisfied TRUE)
            foreach(_alternate_plugin IN LISTS _alternate_plugins)
                if(NOT _alternate_plugin IN_LIST ARG_AVAILABLE)
                    set(_satisfied FALSE)
                endif()
            endforeach()
            if(_satisfied)
                break()
            endif()
        endforeach()
        if(NOT _satisfied)
            list(APPEND _missing "${_plugin}")
        endif()
    endforeach()
    set(${ARG_OUT_VAR}
        "${_missing}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_classify_missing_plugins(REQUESTED <plugin>... AVAILABLE <plugin>... SHIPPED <plugin>...
#                                    OUT_OPTIONAL <var> OUT_ABSENT <var> OUT_UNUSABLE <var>)
# Sorts the REQUESTED plugins that are not AVAILABLE to the build (alternate groups count as in
# qgc_gst_missing_required) into GSTREAMER_OPTIONAL_PLUGINS (OUT_OPTIONAL), the others that the SDK or system does not
# ship at all, i.e. are not in SHIPPED (OUT_ABSENT: misspelled, not built for the platform, or not installed), and the
# rest, shipped but not usable by this build (OUT_UNUSABLE).
function(qgc_gst_classify_missing_plugins)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "OUT_OPTIONAL;OUT_ABSENT;OUT_UNUSABLE" "REQUESTED;AVAILABLE;SHIPPED")
    _qgc_gst_check_args(
        qgc_gst_classify_missing_plugins OUT_OPTIONAL OUT_ABSENT OUT_UNUSABLE ALLOW_EMPTY REQUESTED AVAILABLE SHIPPED
    )
    qgc_gst_missing_required(
        REQUIRED ${ARG_REQUESTED}
        AVAILABLE ${ARG_AVAILABLE}
        OUT_VAR _missing
    )
    set(_optional "")
    set(_absent "")
    set(_unusable "")
    foreach(_plugin IN LISTS _missing)
        qgc_gst_missing_required(
            REQUIRED ${_plugin}
            AVAILABLE ${ARG_SHIPPED}
            OUT_VAR _not_shipped
        )
        if(_plugin IN_LIST GSTREAMER_OPTIONAL_PLUGINS)
            list(APPEND _optional "${_plugin}")
        elseif(_not_shipped)
            list(APPEND _absent "${_plugin}")
        else()
            list(APPEND _unusable "${_plugin}")
        endif()
    endforeach()
    set(${ARG_OUT_OPTIONAL}
        "${_optional}"
        PARENT_SCOPE
    )
    set(${ARG_OUT_ABSENT}
        "${_absent}"
        PARENT_SCOPE
    )
    set(${ARG_OUT_UNUSABLE}
        "${_unusable}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_check_plugins(REQUESTED <plugin>... AVAILABLE <plugin>... SHIPPED <plugin>... SOURCE <where> [STATIC])
# Reports the REQUESTED plugins the build cannot use (qgc_gst_classify_missing_plugins), SOURCE naming where they
# were looked for: a warning for those not shipped at all or shipped without a way to link them, a status line for
# missing GSTREAMER_OPTIONAL_PLUGINS. With STATIC (the mobile layouts, which cannot load a plugin at runtime), a missing
# GSTREAMER_BUNDLE_REQUIRED_PLUGINS entry fails configure.
function(qgc_gst_check_plugins)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "STATIC" "SOURCE" "REQUESTED;AVAILABLE;SHIPPED")
    _qgc_gst_check_args(qgc_gst_check_plugins SOURCE ALLOW_EMPTY REQUESTED AVAILABLE SHIPPED)
    qgc_gst_classify_missing_plugins(
        REQUESTED ${ARG_REQUESTED}
        AVAILABLE ${ARG_AVAILABLE}
        SHIPPED ${ARG_SHIPPED}
        OUT_OPTIONAL _missing_optional
        OUT_ABSENT _absent
        OUT_UNUSABLE _unusable
    )
    if(_absent)
        list(JOIN _absent ", " _absent)
        message(
            WARNING
                "GStreamer: plugin allowlist names not found as plugins in ${ARG_SOURCE}: ${_absent}\nCheck them in "
                "gstreamer.plugins of .github/build-config.json (misspelled, or not built for this platform) or "
                "install the plugins; video features depending on them will not work at runtime."
        )
    endif()
    if(_unusable)
        list(JOIN _unusable ", " _unusable)
        message(
            WARNING "GStreamer: requested plugins in ${ARG_SOURCE} that have no pkg-config module to link them by: "
                    "${_unusable}\nVideo features depending on these plugins will not work at runtime."
        )
    endif()
    if(_missing_optional)
        list(JOIN _missing_optional ", " _missing_optional)
        message(
            STATUS "GStreamer: optional decoder plugins not in ${ARG_SOURCE}: ${_missing_optional} "
                   "(hardware-specific or alternatives; decoding uses those present)"
        )
    endif()
    if(NOT ARG_STATIC)
        return()
    endif()
    qgc_gst_missing_required(
        REQUIRED ${GSTREAMER_BUNDLE_REQUIRED_PLUGINS}
        AVAILABLE ${ARG_AVAILABLE}
        OUT_VAR _missing_required
    )
    if(_missing_required)
        list(JOIN _missing_required ", " _missing_required)
        message(
            FATAL_ERROR "GStreamer: bundle-required plugins are missing from ${ARG_SOURCE}: ${_missing_required}. "
                        "The SDK is incomplete or corrupt — re-download it."
        )
    endif()
endfunction()
