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
set(GSTREAMER_PLUGIN_ALTERNATES
    "videoconvertscale|videoconvert+videoscale"
)

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
set(GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS
    x265
)

# Encode-side plugins needed only by the Debug-only MockLink test video stream
# server (MockVideoStreamServer). Statically registered on Android and iOS Debug
# builds only — desktop builds find them via the full SDK plugin scan, and release
# builds must not ship them (app size; x264/x265 are GPL).
set(GSTREAMER_MOCK_SERVER_DEBUG_PLUGINS
    mpegtsmux videotestsrc x264 x265
)

# Invariant: no plugin may be both runtime-required and xcframework-skipped.
function(_gstreamer_assert_policy_consistent)
    foreach(_skip IN LISTS GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS)
        if(_skip IN_LIST GSTREAMER_RUNTIME_REQUIRED_PLUGINS)
            message(FATAL_ERROR
                "GStreamer policy: plugin '${_skip}' is in both GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS "
                "and GSTREAMER_RUNTIME_REQUIRED_PLUGINS — a required plugin cannot be skipped on iOS.")
        endif()
    endforeach()
endfunction()
_gstreamer_assert_policy_consistent()

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
            "expected QGC_BUILD_CONFIG_CONTENT to be set by BuildConfig.cmake.")
    endif()
    _qgc_json_array_to_list(_plugins "${QGC_BUILD_CONFIG_CONTENT}" gstreamer plugins common)
    if(ARG_PLATFORM)
        _qgc_json_array_to_list(_extra "${QGC_BUILD_CONFIG_CONTENT}" gstreamer plugins ${ARG_PLATFORM})
        list(APPEND _plugins ${_extra})
    endif()
    set(${ARG_OUT_VAR} "${_plugins}" PARENT_SCOPE)
endfunction()

# gstreamer_runtime_required_plugins(OUT_VAR)
function(gstreamer_runtime_required_plugins OUT_VAR)
    set(${OUT_VAR} "${GSTREAMER_RUNTIME_REQUIRED_PLUGINS}" PARENT_SCOPE)
endfunction()

# gstreamer_plugin_satisfy_sets(PLUGIN <name> OUT_VAR <var>)
# Returns the alternate-satisfaction sets for <name>: a ";"-list where each entry
# is a "+"-joined set of plugin basenames that together satisfy <name>. If <name>
# belongs to an alternate group, returns that group's alternatives; otherwise just
# "<name>". Lets the install verifier accept videoconvert+videoscale (1.20) in
# place of the fused videoconvertscale (1.22+) instead of failing the build.
function(gstreamer_plugin_satisfy_sets)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "PLUGIN;OUT_VAR" "")
    if(ARG_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR
            "gstreamer_plugin_satisfy_sets: missing values for: ${ARG_KEYWORDS_MISSING_VALUES}")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "gstreamer_plugin_satisfy_sets: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_PLUGIN)
        message(FATAL_ERROR "gstreamer_plugin_satisfy_sets: PLUGIN is required")
    endif()
    if(NOT ARG_OUT_VAR)
        message(FATAL_ERROR "gstreamer_plugin_satisfy_sets: OUT_VAR is required")
    endif()
    foreach(_group IN LISTS GSTREAMER_PLUGIN_ALTERNATES)
        string(REPLACE "|" ";" _alts "${_group}")
        foreach(_alt IN LISTS _alts)
            string(REPLACE "+" ";" _members "${_alt}")
            if(ARG_PLUGIN IN_LIST _members)
                set(${ARG_OUT_VAR} "${_alts}" PARENT_SCOPE)
                return()
            endif()
        endforeach()
    endforeach()
    set(${ARG_OUT_VAR} "${ARG_PLUGIN}" PARENT_SCOPE)
endfunction()

# gstreamer_plugin_satisfied(PLUGIN <name> AVAILABLE <plugin>... OUT_VAR <var>)
# Sets <var> to TRUE when <name> is in AVAILABLE or one of its alternate sets
# (gstreamer_plugin_satisfy_sets) is entirely in AVAILABLE, else FALSE.
function(gstreamer_plugin_satisfied)
    cmake_parse_arguments(PARSE_ARGV 0 ARG "" "PLUGIN;OUT_VAR" "AVAILABLE")
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "gstreamer_plugin_satisfied: unknown arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif()
    if(NOT ARG_PLUGIN OR NOT ARG_OUT_VAR)
        message(FATAL_ERROR "gstreamer_plugin_satisfied: PLUGIN and OUT_VAR are required")
    endif()
    set(_satisfied FALSE)
    if(ARG_PLUGIN IN_LIST ARG_AVAILABLE)
        set(_satisfied TRUE)
    else()
        gstreamer_plugin_satisfy_sets(PLUGIN "${ARG_PLUGIN}" OUT_VAR _sets)
        foreach(_set IN LISTS _sets)
            string(REPLACE "+" ";" _members "${_set}")
            set(_set_available TRUE)
            foreach(_member IN LISTS _members)
                if(NOT _member IN_LIST ARG_AVAILABLE)
                    set(_set_available FALSE)
                    break()
                endif()
            endforeach()
            if(_set_available)
                set(_satisfied TRUE)
                break()
            endif()
        endforeach()
    endif()
    set(${ARG_OUT_VAR}
        ${_satisfied}
        PARENT_SCOPE
    )
endfunction()

# gstreamer_missing_required(REQUIRED <plugin>... AVAILABLE <plugin>... OUT_VAR <var>)
# Sets <var> to the REQUIRED plugins, in order, that gstreamer_plugin_satisfied
# rejects against AVAILABLE.
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
        gstreamer_plugin_satisfied(PLUGIN "${_plugin}" AVAILABLE ${ARG_AVAILABLE} OUT_VAR _satisfied)
        if(NOT _satisfied)
            list(APPEND _missing "${_plugin}")
        endif()
    endforeach()
    set(${ARG_OUT_VAR}
        "${_missing}"
        PARENT_SCOPE
    )
endfunction()

# gstreamer_xcfw_skip(OUT_VAR)
function(gstreamer_xcfw_skip OUT_VAR)
    set(${OUT_VAR} "${GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS}" PARENT_SCOPE)
endfunction()
