# ============================================================================
# BuildConfig.cmake - Read .github/build-config.json
# ============================================================================

include_guard(GLOBAL)

set(QGC_BUILD_CONFIG_FILE "${CMAKE_SOURCE_DIR}/.github/build-config.json")

if(NOT EXISTS "${QGC_BUILD_CONFIG_FILE}")
    message(FATAL_ERROR "QGC: BuildConfig: Config file not found: ${QGC_BUILD_CONFIG_FILE}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${QGC_BUILD_CONFIG_FILE}")

file(READ "${QGC_BUILD_CONFIG_FILE}" QGC_BUILD_CONFIG_CONTENT)

# Extract value from JSON using CMake's native JSON parser; supports dotted paths
function(qgc_config_get_value VAR_NAME JSON_KEY)
    if(NOT ARGC EQUAL 2
       OR NOT VAR_NAME MATCHES "^[A-Za-z_][A-Za-z0-9_]*$"
       OR NOT JSON_KEY
    )
        message(FATAL_ERROR
            "qgc_config_get_value: a valid output variable and non-empty JSON key are required")
    endif()
    string(REPLACE "." ";" _path "${JSON_KEY}")
    string(JSON _type ERROR_VARIABLE _err TYPE "${QGC_BUILD_CONFIG_CONTENT}" ${_path})
    if(_err)
        message(FATAL_ERROR "QGC: BuildConfig: Key '${JSON_KEY}' not found in ${QGC_BUILD_CONFIG_FILE}")
    endif()
    if(_type STREQUAL "ARRAY" OR _type STREQUAL "OBJECT")
        message(FATAL_ERROR
            "QGC: BuildConfig: Key '${JSON_KEY}' must be scalar, got ${_type}")
    endif()
    string(JSON _value ERROR_VARIABLE _err GET "${QGC_BUILD_CONFIG_CONTENT}" ${_path})
    if(_err)
        message(FATAL_ERROR "QGC: BuildConfig: Key '${JSON_KEY}' not found in ${QGC_BUILD_CONFIG_FILE}")
    endif()
    set(${VAR_NAME} "${_value}" CACHE STRING "${JSON_KEY}" FORCE)
endfunction()

qgc_config_get_value(QGC_CONFIG_QT_VERSION "qt.version")
qgc_config_get_value(QGC_CONFIG_QT_MINIMUM_VERSION "qt.minimum_version")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_VERSION         "gstreamer.version.default")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_MIN_VERSION     "gstreamer.version.minimum")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_ANDROID_VERSION "gstreamer.version.android")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_IOS_VERSION     "gstreamer.version.ios")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_MACOS_VERSION   "gstreamer.version.macos")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_WIN_VERSION     "gstreamer.version.windows")
qgc_config_get_value(QGC_CONFIG_NDK_VERSION "android.ndk_version")
qgc_config_get_value(QGC_CONFIG_NDK_FULL_VERSION "android.ndk_full_version")
qgc_config_get_value(QGC_CONFIG_JAVA_VERSION "android.java_version")
qgc_config_get_value(QGC_CONFIG_ANDROID_PLATFORM "android.platform")
qgc_config_get_value(QGC_CONFIG_ANDROID_MIN_SDK "android.min_sdk")
qgc_config_get_value(QGC_CONFIG_CMAKE_MINIMUM "build.cmake_minimum_version")
qgc_config_get_value(QGC_CONFIG_MACOS_DEPLOYMENT_TARGET "apple.macos_deployment_target")
qgc_config_get_value(QGC_CONFIG_IOS_DEPLOYMENT_TARGET "apple.ios_deployment_target")

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
