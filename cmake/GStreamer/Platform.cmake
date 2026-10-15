# Target platforms of the GStreamer modules: qgc_gst_platform names the platform being built, and
# qgc_gst_platform_descriptor holds the one row per platform that the Orchestrator and the install helpers dispatch on.

include_guard(GLOBAL)

# qgc_gst_platform(<out_var>)
# Sets <out_var> to the target platform: windows, linux, android, macos or ios; "" when unsupported.
function(qgc_gst_platform out_var)
    if(ANDROID)
        set(_platform android)
    elseif(IOS)
        set(_platform ios)
    elseif(WIN32)
        set(_platform windows)
    elseif(MACOS)
        set(_platform macos)
    elseif(LINUX)
        set(_platform linux)
    else()
        set(_platform "")
    endif()
    set(${out_var}
        "${_platform}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_platform_descriptor(<platform> <prefix>)
# Sets, in the caller's scope:
#   <prefix>_VERSION_VAR  cmake/BuildConfig.cmake variable holding the platform's gstreamer.version entry
#   <prefix>_PLUGIN_KEY   gstreamer.plugins.<key> list in build-config.json added to the common plugins
#   <prefix>_DISCOVER     SDK discovery command (platform/*.cmake)
#   <prefix>_INSTALL      install command (Install.cmake), called as <command>(<app_name> <plugin_dest_out>) and
#                         returning the plugin directory it installs to; "" on mobile, where plugins link statically
# cmakelang 0.6.13 predates return(PROPAGATE).
# cmake-lint: disable=C0103,E1121
function(qgc_gst_platform_descriptor platform prefix)
    # platform|version variable|plugin key|discovery|installer
    set(_rows
        "linux|QGC_CONFIG_GSTREAMER_MIN_VERSION|linux|_qgc_gst_discover_linux_sdk|gstreamer_install_linux_sdk"
        "windows|QGC_CONFIG_GSTREAMER_WIN_VERSION|windows|_qgc_gst_discover_windows_sdk|gstreamer_install_windows_sdk"
        "macos|QGC_CONFIG_GSTREAMER_MACOS_VERSION|apple|_qgc_gst_discover_macos_sdk|gstreamer_install_macos_sdk"
        "android|QGC_CONFIG_GSTREAMER_ANDROID_VERSION|android|_qgc_gst_discover_android_sdk|"
        "ios|QGC_CONFIG_GSTREAMER_IOS_VERSION|apple|_qgc_gst_discover_ios_sdk|"
    )
    foreach(_row IN LISTS _rows)
        string(REPLACE "|" ";" _fields "${_row}")
        list(GET _fields 0 _name)
        if(_name STREQUAL platform)
            list(GET _fields 1 ${prefix}_VERSION_VAR)
            list(GET _fields 2 ${prefix}_PLUGIN_KEY)
            list(GET _fields 3 ${prefix}_DISCOVER)
            list(GET _fields 4 ${prefix}_INSTALL)
            return(PROPAGATE ${prefix}_VERSION_VAR ${prefix}_PLUGIN_KEY ${prefix}_DISCOVER ${prefix}_INSTALL)
        endif()
    endforeach()
    message(FATAL_ERROR "GStreamer: unsupported target platform '${platform}' (CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})")
endfunction()
