# ============================================================================
# BuildConfig.cmake - Read .github/build-config.json
# ============================================================================

set(QGC_BUILD_CONFIG_FILE "${CMAKE_SOURCE_DIR}/.github/build-config.json")

if(NOT EXISTS "${QGC_BUILD_CONFIG_FILE}")
    message(FATAL_ERROR "BuildConfig: Config file not found: ${QGC_BUILD_CONFIG_FILE}")
endif()

# Read the JSON file
file(READ "${QGC_BUILD_CONFIG_FILE}" QGC_BUILD_CONFIG_CONTENT)

# Extract value from JSON using CMake's native JSON parser
function(qgc_config_get_value VAR_NAME JSON_KEY)
    string(JSON _value ERROR_VARIABLE _err GET "${QGC_BUILD_CONFIG_CONTENT}" "${JSON_KEY}")
    if(_err)
        message(FATAL_ERROR "BuildConfig: Key '${JSON_KEY}' not found in ${QGC_BUILD_CONFIG_FILE}")
    endif()
    set(${VAR_NAME} "${_value}" CACHE STRING "${JSON_KEY}" FORCE)
endfunction()

qgc_config_get_value(QGC_CONFIG_QT_VERSION "qt_version")
qgc_config_get_value(QGC_CONFIG_QT_MINIMUM_VERSION "qt_minimum_version")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_VERSION "gstreamer_version")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_ANDROID_VERSION "gstreamer_android_version")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_MACOS_VERSION "gstreamer_macos_version")
qgc_config_get_value(QGC_CONFIG_GSTREAMER_WIN_VERSION "gstreamer_windows_version")
qgc_config_get_value(QGC_CONFIG_NDK_VERSION "ndk_version")
qgc_config_get_value(QGC_CONFIG_NDK_FULL_VERSION "ndk_full_version")
qgc_config_get_value(QGC_CONFIG_JAVA_VERSION "java_version")
qgc_config_get_value(QGC_CONFIG_ANDROID_PLATFORM "android_platform")
qgc_config_get_value(QGC_CONFIG_ANDROID_MIN_SDK "android_min_sdk")
qgc_config_get_value(QGC_CONFIG_CMAKE_MINIMUM "cmake_minimum_version")
qgc_config_get_value(QGC_CONFIG_MACOS_DEPLOYMENT_TARGET "macos_deployment_target")
qgc_config_get_value(QGC_CONFIG_IOS_DEPLOYMENT_TARGET "ios_deployment_target")

message(STATUS "BuildConfig: Qt ${QGC_CONFIG_QT_VERSION}, GStreamer ${QGC_CONFIG_GSTREAMER_VERSION}, NDK ${QGC_CONFIG_NDK_VERSION}")
