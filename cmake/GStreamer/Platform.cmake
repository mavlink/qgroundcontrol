# Target platform of the GStreamer modules. A platform's SDK discovery is _qgc_gst_discover_<platform>_sdk
# (platform/*.cmake) and, on desktop, its installer gstreamer_install_<platform>_sdk (Install.cmake).
#
# A discovery function sets, in the Orchestrator's scope, GStreamer_ROOT_DIR (the SDK it resolved),
# GStreamer_AUTO_DOWNLOADED when it downloaded that SDK, its GSTREAMER_{LIB,PLUGIN,INCLUDE}_PATH layout
# (gstreamer_create_layout_target) and its pkg-config view (gstreamer_apply_pkgconfig_env: the environment, plus
# PKG_CONFIG_EXECUTABLE and PKG_CONFIG_ARGN where the SDK brings its own), and overrides the GStreamer_USE_* layout
# preferences when the SDK it found differs.

include_guard(GLOBAL)

# qgc_gst_platform(<prefix>)
# Sets, in the caller's scope:
#   <prefix>              the target platform: windows, linux, android, macos or ios
#   <prefix>_VERSION_KEY  gstreamer.version.<key> in build-config.json: the SDK version, or on Linux the floor
#   <prefix>_PLUGIN_KEY   gstreamer.plugins.<key> list in build-config.json added to the common plugins
# Fails configure on any other platform.
# cmake-lint: disable=C0103,E1121
function(qgc_gst_platform prefix)
    if(ANDROID)
        set(${prefix} android)
        set(${prefix}_VERSION_KEY android)
        set(${prefix}_PLUGIN_KEY android)
    elseif(IOS)
        set(${prefix} ios)
        set(${prefix}_VERSION_KEY ios)
        set(${prefix}_PLUGIN_KEY apple)
    elseif(WIN32)
        set(${prefix} windows)
        set(${prefix}_VERSION_KEY windows)
        set(${prefix}_PLUGIN_KEY windows)
    elseif(MACOS)
        set(${prefix} macos)
        set(${prefix}_VERSION_KEY macos)
        set(${prefix}_PLUGIN_KEY apple)
    elseif(LINUX)
        set(${prefix} linux)
        set(${prefix}_VERSION_KEY minimum)
        set(${prefix}_PLUGIN_KEY linux)
    else()
        message(FATAL_ERROR "GStreamer: unsupported target platform (CMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME})")
    endif()
    return(PROPAGATE ${prefix} ${prefix}_VERSION_KEY ${prefix}_PLUGIN_KEY)
endfunction()
