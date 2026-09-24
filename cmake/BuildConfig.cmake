# ============================================================================
# BuildConfig.cmake - Build configuration (hardcoded for offline tarball builds)
# ============================================================================

include_guard(GLOBAL)

# Hardcoded build configuration. The gear-built tarball has no .git, and the
# .github directory is export-ignored, so .github/build-config.json is absent.
set(QGC_CONFIG_QT_VERSION "6.11.2" CACHE STRING "qt.version")
set(QGC_CONFIG_QT_MINIMUM_VERSION "6.11.0" CACHE STRING "qt.minimum_version")
set(QGC_CONFIG_GSTREAMER_VERSION "1.28.4" CACHE STRING "gstreamer.version.default")
set(QGC_CONFIG_GSTREAMER_MIN_VERSION "1.20.0" CACHE STRING "gstreamer.version.minimum")
set(QGC_CONFIG_GSTREAMER_ANDROID_VERSION "1.28.4" CACHE STRING "gstreamer.version.android")
set(QGC_CONFIG_GSTREAMER_IOS_VERSION "1.28.4" CACHE STRING "gstreamer.version.ios")
set(QGC_CONFIG_GSTREAMER_MACOS_VERSION "1.28.4" CACHE STRING "gstreamer.version.macos")
set(QGC_CONFIG_GSTREAMER_WIN_VERSION "1.28.4" CACHE STRING "gstreamer.version.windows")
set(QGC_CONFIG_NDK_VERSION "r27c" CACHE STRING "android.ndk_version")
set(QGC_CONFIG_NDK_FULL_VERSION "27.2.12479018" CACHE STRING "android.ndk_full_version")
set(QGC_CONFIG_JAVA_VERSION "21" CACHE STRING "android.java_version")
set(QGC_CONFIG_ANDROID_PLATFORM "36" CACHE STRING "android.platform")
set(QGC_CONFIG_ANDROID_MIN_SDK "28" CACHE STRING "android.min_sdk")
set(QGC_CONFIG_CMAKE_MINIMUM "3.25" CACHE STRING "build.cmake_minimum_version")
set(QGC_CONFIG_MACOS_DEPLOYMENT_TARGET "13.0" CACHE STRING "apple.macos_deployment_target")
set(QGC_CONFIG_IOS_DEPLOYMENT_TARGET "17.0" CACHE STRING "apple.ios_deployment_target")

# Minimal build-config JSON: the GStreamer plugin policy parses
# gstreamer.plugins.common + gstreamer.plugins.<platform> from
# QGC_BUILD_CONFIG_CONTENT (cmake/GStreamer/PluginPolicy.cmake).
set(QGC_BUILD_CONFIG_CONTENT [=[
{"gstreamer":{"plugins":{"common":["app","coreelements","isomp4","libav","matroska","mpegtsdemux","multifile","opengl","openh264","playback","rtp","rtpmanager","rtsp","sdpelem","tcp","typefindfunctions","udp","videoparsersbad","vpx","videoconvertscale","videoconvert","videoscale"],"android":["androidmedia","dav1d"],"apple":["applemedia","dav1d"],"windows":["d3d","d3d11","d3d12","dav1d","nvcodec"],"linux":["nvcodec","qsv","va","vulkan"]}}}
]=] CACHE STRING "build-config.json content (gstreamer plugins)")

# Extract patch versions from platform strings (e.g., "1.22.12" -> QGC_GSTREAMER_PATCH_1_22=12)
foreach(_gst_ver_var QGC_CONFIG_GSTREAMER_ANDROID_VERSION QGC_CONFIG_GSTREAMER_IOS_VERSION QGC_CONFIG_GSTREAMER_MACOS_VERSION QGC_CONFIG_GSTREAMER_WIN_VERSION)
    if(DEFINED ${_gst_ver_var})
        string(REPLACE "." ";" _ver_parts "${${_gst_ver_var}}")
        list(LENGTH _ver_parts _ver_len)
        if(_ver_len EQUAL 3)
            list(GET _ver_parts 0 _major)
            list(GET _ver_parts 1 _minor)
            list(GET _ver_parts 2 _patch)
            set(_patch_var "QGC_GSTREAMER_PATCH_${_major}_${_minor}")
            if(NOT DEFINED ${_patch_var})
                set(${_patch_var} "${_patch}" CACHE STRING "GStreamer ${_major}.${_minor} patch version (from ${_gst_ver_var})")
            endif()
        endif()
    endif()
endforeach()

message(STATUS "BuildConfig: Qt ${QGC_CONFIG_QT_VERSION}, GStreamer ${QGC_CONFIG_GSTREAMER_VERSION}, NDK ${QGC_CONFIG_NDK_VERSION}")
