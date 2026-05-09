# Centralised GStreamer plugin policy — single source of truth for plugin
# alternate groups, runtime-required plugins, and xcframework skip list.
# Consumed by Orchestrator (build-config plugin loader / alternate-satisfied
# check), Install (runtime-required verifier), and platform/IOS (xcfw skip).

include_guard(GLOBAL)

# Plugin alternate groups — within each group, only one member typically ships
# per SDK (e.g. videoconvertscale is the 1.22 fusion of videoconvert+videoscale).
# Format: each entry is a "|"-separated alternate set; member sub-groups are
# "+"-separated for AND-satisfaction. Example: "videoconvertscale|videoconvert+videoscale"
# means satisfied if videoconvertscale is present OR if both videoconvert AND
# videoscale are present.
set(GSTREAMER_PLUGIN_ALTERNATES
    "videoconvertscale|videoconvert+videoscale"
)

# Plugins guaranteed to exist post-install — verifier in
# gstreamer_install_platform_sdk fails the build if any are missing.
set(GSTREAMER_RUNTIME_REQUIRED_PLUGINS
    coreelements playback rtsp rtp rtpmanager tcp udp
)

# iOS xcframework: plugins whose dependent static libs aren't bundled in the
# slice. Auto-registering them would cause unresolved-symbol link failures.
# x265: gstreamer-ios 1.28.2 ships libgstx265.a but not libx265.a.
set(GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS
    x265
)

# gstreamer_plugins_for(PLATFORM <key> OUT_VAR <var>)
# Reads the plugin list for <key> ("android"|"apple"|"windows"|"linux"|"") from
# build-config.json (gstreamer.plugins.common + gstreamer.plugins.<key>).
# Requires QGC_BUILD_CONFIG_CONTENT to be set by BuildConfig.cmake.
function(gstreamer_plugins_for)
    cmake_parse_arguments(ARG "" "PLATFORM;OUT_VAR" "" ${ARGN})
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

# gstreamer_current_platform_key(OUT_VAR)
# Maps the current CMake platform booleans to the build-config.json key.
function(gstreamer_current_platform_key OUT_VAR)
    if(ANDROID)
        set(_key android)
    elseif(APPLE)
        set(_key apple)
    elseif(WIN32)
        set(_key windows)
    elseif(LINUX)
        set(_key linux)
    else()
        set(_key "")
    endif()
    set(${OUT_VAR} "${_key}" PARENT_SCOPE)
endfunction()

# gstreamer_filter_alternates(IN_OUT_PLUGINS <var> AVAILABLE <list>)
# Removes plugins belonging to satisfied alternate groups from the requested
# list, suppressing false missing-plugin warnings when only one member of an
# alternate set ships in the SDK. Modifies the variable named by IN_OUT_PLUGINS
# in the caller's scope.
function(gstreamer_filter_alternates)
    cmake_parse_arguments(ARG "" "IN_OUT_PLUGINS" "AVAILABLE" ${ARGN})
    if(NOT ARG_IN_OUT_PLUGINS)
        message(FATAL_ERROR "gstreamer_filter_alternates: IN_OUT_PLUGINS is required")
    endif()
    set(_plugins "${${ARG_IN_OUT_PLUGINS}}")
    foreach(_group IN LISTS GSTREAMER_PLUGIN_ALTERNATES)
        string(REPLACE "|" ";" _alts "${_group}")
        set(_satisfied FALSE)
        set(_all_members "")
        foreach(_alt IN LISTS _alts)
            string(REPLACE "+" ";" _members "${_alt}")
            list(APPEND _all_members ${_members})
            set(_alt_ok TRUE)
            foreach(_m IN LISTS _members)
                if(NOT _m IN_LIST ARG_AVAILABLE)
                    set(_alt_ok FALSE)
                    break()
                endif()
            endforeach()
            if(_alt_ok)
                set(_satisfied TRUE)
            endif()
        endforeach()
        if(_satisfied)
            list(REMOVE_DUPLICATES _all_members)
            foreach(_m IN LISTS _all_members)
                list(REMOVE_ITEM _plugins "${_m}")
            endforeach()
        endif()
    endforeach()
    set(${ARG_IN_OUT_PLUGINS} "${_plugins}" PARENT_SCOPE)
endfunction()

# gstreamer_runtime_required_plugins(OUT_VAR)
function(gstreamer_runtime_required_plugins OUT_VAR)
    set(${OUT_VAR} "${GSTREAMER_RUNTIME_REQUIRED_PLUGINS}" PARENT_SCOPE)
endfunction()

# gstreamer_xcfw_skip(OUT_VAR)
function(gstreamer_xcfw_skip OUT_VAR)
    set(${OUT_VAR} "${GSTREAMER_XCFRAMEWORK_SKIP_PLUGINS}" PARENT_SCOPE)
endfunction()
