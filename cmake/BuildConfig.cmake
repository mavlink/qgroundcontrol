# ============================================================================
# BuildConfig.cmake - Read .github/build-config.json
# ============================================================================

include_guard(GLOBAL)

set(QGC_BUILD_CONFIG_FILE "${CMAKE_SOURCE_DIR}/.github/build-config.json")

if(NOT EXISTS "${QGC_BUILD_CONFIG_FILE}")
    message(FATAL_ERROR "QGC: BuildConfig: Config file not found: ${QGC_BUILD_CONFIG_FILE}")
endif()
set_property(
    DIRECTORY
    APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS "${QGC_BUILD_CONFIG_FILE}"
)

file(READ "${QGC_BUILD_CONFIG_FILE}" QGC_BUILD_CONFIG_CONTENT)

include("${CMAKE_CURRENT_LIST_DIR}/modules/BuildConfigReader.cmake")

qgc_config_get_value(QGC_CONFIG_QT_VERSION "qt.version")
qgc_config_get_value(QGC_CONFIG_QT_MINIMUM_VERSION "qt.minimum_version")
qgc_config_get_value(QGC_CONFIG_NDK_VERSION "android.ndk_version")
qgc_config_get_value(QGC_CONFIG_NDK_FULL_VERSION "android.ndk_full_version")
qgc_config_get_value(QGC_CONFIG_JAVA_VERSION "android.java_version")
qgc_config_get_value(QGC_CONFIG_ANDROID_PLATFORM "android.platform")
qgc_config_get_value(QGC_CONFIG_ANDROID_MIN_SDK "android.min_sdk")
qgc_config_get_value(QGC_CONFIG_CMAKE_MINIMUM "build.cmake_minimum_version")
qgc_config_get_value(QGC_CONFIG_MACOS_DEPLOYMENT_TARGET "apple.macos_deployment_target")
qgc_config_get_value(QGC_CONFIG_IOS_DEPLOYMENT_TARGET "apple.ios_deployment_target")

# The GStreamer version depends on the platform (Linux: a floor); cmake/GStreamer/Orchestrator.cmake prints it.
message(STATUS "BuildConfig: Qt ${QGC_CONFIG_QT_VERSION}, NDK ${QGC_CONFIG_NDK_VERSION}")
