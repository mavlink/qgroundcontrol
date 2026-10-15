# Target platform of the GStreamer modules. A platform's SDK discovery is _qgc_gst_discover_<platform>_sdk
# (platform/*.cmake) and, on desktop, its installer gstreamer_install_<platform>_sdk (Install.cmake).

include_guard(GLOBAL)

# qgc_gst_platform(<prefix>)
# Sets, in the caller's scope:
#   <prefix>              the target platform: windows, linux, android, macos or ios
#   <prefix>_VERSION_VAR  cmake/BuildConfig.cmake variable holding the platform's gstreamer.version entry
#   <prefix>_PLUGIN_KEY   gstreamer.plugins.<key> list in build-config.json added to the common plugins
# Fails configure on any other platform.
# cmakelang 0.6.13 predates return(PROPAGATE).
# cmake-lint: disable=C0103,E1121
function(qgc_gst_platform prefix)
    if(ANDROID)
        set(${prefix} android)
        set(${prefix}_VERSION_VAR QGC_CONFIG_GSTREAMER_ANDROID_VERSION)
        set(${prefix}_PLUGIN_KEY android)
    elseif(IOS)
        set(${prefix} ios)
        set(${prefix}_VERSION_VAR QGC_CONFIG_GSTREAMER_IOS_VERSION)
        set(${prefix}_PLUGIN_KEY apple)
    elseif(WIN32)
        set(${prefix} windows)
        set(${prefix}_VERSION_VAR QGC_CONFIG_GSTREAMER_WIN_VERSION)
        set(${prefix}_PLUGIN_KEY windows)
    elseif(MACOS)
        set(${prefix} macos)
        set(${prefix}_VERSION_VAR QGC_CONFIG_GSTREAMER_MACOS_VERSION)
        set(${prefix}_PLUGIN_KEY apple)
    elseif(LINUX)
        set(${prefix} linux)
        set(${prefix}_VERSION_VAR QGC_CONFIG_GSTREAMER_MIN_VERSION)
        set(${prefix}_PLUGIN_KEY linux)
    else()
        message(FATAL_ERROR "GStreamer: unsupported target platform (CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})")
    endif()
    return(PROPAGATE ${prefix} ${prefix}_VERSION_VAR ${prefix}_PLUGIN_KEY)
endfunction()
