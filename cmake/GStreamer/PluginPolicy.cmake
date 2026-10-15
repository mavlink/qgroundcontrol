# Centralised GStreamer plugin policy — single source of truth for plugin
# alternate groups, runtime-required plugins, and xcframework skip list.
# Consumed by Orchestrator (build-config plugin loader / alternate-satisfied
# check), Install (runtime-required verifier), and the iOS plugin list (xcfw skip).

include_guard(GLOBAL)

if(POLICY CMP0174)
    cmake_policy(SET CMP0174 NEW)
endif()

# Plugin alternate groups — within each group, only one member typically ships
# per SDK (e.g. videoconvertscale is the 1.22 fusion of videoconvert+videoscale).
# Format: each entry is a "|"-separated alternate set; member sub-groups are
# "+"-separated for AND-satisfaction. Example: "videoconvertscale|videoconvert+videoscale"
# means satisfied if videoconvertscale is present OR if both videoconvert AND
# videoscale are present. Plugin basenames must not contain '|' or '+' — those
# are the group/AND delimiters and are not escapable.
set(GSTREAMER_PLUGIN_ALTERNATES "videoconvertscale|videoconvert+videoscale")

# Structural plugins guaranteed to exist post-install — the install verifier
# fails the build if any are missing; _verifyPlugins() (GStreamer.cc) mirrors this list.
# Codec implementations (openh264/libav) are intentionally excluded: distro
# bundles may omit one while shipping another usable decode path.
# opengl supplies glupload/glcolorconvert (GL zero-copy video path);
# multifile (splitmuxsink), isomp4 (qtmux/mp4mux), and matroska (matroskamux)
# are load-bearing for video recording (GstVideoReceiver _kFileMux).
set(GSTREAMER_RUNTIME_REQUIRED_PLUGINS
    coreelements
    isomp4
    matroska
    multifile
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

# iOS xcframework: plugins whose dependent static libs aren't bundled in the
# slice. Registering them would cause unresolved-symbol link failures, so the
# Orchestrator drops them from the iOS plugin list.
# x265: gstreamer-ios 1.28.2 ships libgstx265.a but not libx265.a.
set(GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS x265)

# Encode-side plugins needed only by the Debug-only MockLink test video stream
# server (MockVideoStreamServer). Statically registered on Android and iOS Debug
# builds only — desktop builds find them via the full SDK plugin scan, and release
# builds must not ship them (app size; x264/x265 are GPL).
set(GSTREAMER_MOCK_SERVER_DEBUG_PLUGINS mpegtsmux videotestsrc x264 x265)

# Invariant: no plugin may be both runtime-required and xcframework-skipped.
function(_qgc_gst_assert_policy_consistent)
    foreach(_skip IN LISTS GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS)
        if(_skip IN_LIST GSTREAMER_RUNTIME_REQUIRED_PLUGINS)
            message(FATAL_ERROR "GStreamer policy: plugin '${_skip}' is in both GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS "
                                "and GSTREAMER_RUNTIME_REQUIRED_PLUGINS — a required plugin cannot be skipped on iOS."
            )
        endif()
    endforeach()
endfunction()
_qgc_gst_assert_policy_consistent()

# gstreamer_plugins_for(PLATFORM <key> OUT_VAR <var>)
# Reads the plugin list for <key> ("android"|"apple"|"windows"|"linux"|"") from
# build-config.json (gstreamer.plugins.common + gstreamer.plugins.<key>).
# Requires QGC_BUILD_CONFIG_CONTENT to be set by BuildConfig.cmake.
function(gstreamer_plugins_for)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "PLATFORM;OUT_VAR" "")
    set(_missing_values ${ARG_KEYWORDS_MISSING_VALUES})
    list(REMOVE_ITEM _missing_values PLATFORM)
    if(_missing_values)
        message(FATAL_ERROR "gstreamer_plugins_for: missing values for: ${_missing_values}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "gstreamer_plugins_for: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_OUT_VAR)
        message(FATAL_ERROR "gstreamer_plugins_for: OUT_VAR is required")
    endif()
    if(NOT DEFINED QGC_BUILD_CONFIG_CONTENT)
        message(FATAL_ERROR "gstreamer_plugins_for: build-config.json not loaded; "
                            "expected QGC_BUILD_CONFIG_CONTENT to be set by BuildConfig.cmake."
        )
    endif()
    _qgc_gst_json_array_to_list(_plugins "${QGC_BUILD_CONFIG_CONTENT}" gstreamer plugins common)
    if(ARG_PLATFORM)
        _qgc_gst_json_array_to_list(_extra "${QGC_BUILD_CONFIG_CONTENT}" gstreamer plugins ${ARG_PLATFORM})
        list(APPEND _plugins ${_extra})
    endif()
    set(${ARG_OUT_VAR}
        "${_plugins}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_runtime_required_plugins(<out_var>)
function(gstreamer_runtime_required_plugins out_var)
    set(${out_var}
        "${GSTREAMER_RUNTIME_REQUIRED_PLUGINS}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_missing_required(REQUIRED <plugin>... AVAILABLE <plugin>... OUT_VAR <var>)
# Sets <var> to the REQUIRED plugins, in order, that are not AVAILABLE. A member of a GSTREAMER_PLUGIN_ALTERNATES group
# is also satisfied by any alternate of its group whose plugins are all AVAILABLE, so the install verifier accepts
# videoconvert+videoscale (1.20) in place of the fused videoconvertscale (1.22+).
function(gstreamer_missing_required)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "OUT_VAR" "REQUIRED;AVAILABLE")
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "gstreamer_missing_required: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_OUT_VAR)
        message(FATAL_ERROR "gstreamer_missing_required: OUT_VAR is required")
    endif()
    set(_missing "")
    foreach(_plugin IN LISTS ARG_REQUIRED)
        set(_alternates "${_plugin}")
        foreach(_group IN LISTS GSTREAMER_PLUGIN_ALTERNATES)
            string(REGEX REPLACE "[|+]" ";" _group_plugins "${_group}")
            if(_plugin IN_LIST _group_plugins)
                string(REPLACE "|" ";" _alternates "${_plugin}|${_group}")
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

# gstreamer_xcfw_skip(<out_var>)
function(gstreamer_xcfw_skip out_var)
    set(${out_var}
        "${GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS}"
        PARENT_SCOPE
    )
endfunction()
