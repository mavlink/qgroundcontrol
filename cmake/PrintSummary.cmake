# ----------------------------------------------------------------------------
# Print a timestamp for reproducibility
string(TIMESTAMP QGC_CONFIGURE_TIME "%Y-%m-%d %H:%M:%S %Z")
message(STATUS "QGroundControl configuration generated at ${QGC_CONFIGURE_TIME}")
message(STATUS "==================================================================")

# ----------------------------------------------------------------------------
# Macro for printing ON/OFF flags
# ----------------------------------------------------------------------------
macro(OptionOutput _label)
    if(${ARGN})
        set(_val "ON")
    else()
        set(_val "OFF")
    endif()
    message(STATUS "${_label}: ${_val}")
endmacro()

# ----------------------------------------------------------------------------
# Imported CMake variables
# ----------------------------------------------------------------------------
message(STATUS "-- CMake System -----------------------------------------------------")
message(STATUS "Install prefix:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Generator:            ${CMAKE_GENERATOR} ${CMAKE_GENERATOR_PLATFORM} ${CMAKE_GENERATOR_TOOLSET}")
message(STATUS "Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "Host system:          ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION}")
message(STATUS "Target system:        ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "CMake version:        ${CMAKE_VERSION}")
message(STATUS "Source directory:     ${CMAKE_SOURCE_DIR}")
message(STATUS "Toolchain file:       ${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "Prefix path:          ${CMAKE_PREFIX_PATH}")
message(STATUS "------------------------------------------------------------------")

message(STATUS "-- Compiler & Linker -----------------------------------------------")
message(STATUS "C++ compiler:         ${CMAKE_CXX_COMPILER_ID} (${CMAKE_CXX_COMPILER}) v${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "C++ standard:         C++${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler flags:       ${CMAKE_CXX_FLAGS}")
message(STATUS "  Debug:              ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "  Release:            ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "Linker flags:         ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "  Debug:              ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
message(STATUS "  Release:            ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
message(STATUS "------------------------------------------------------------------")

# ----------------------------------------------------------------------------
# App metadata
# ----------------------------------------------------------------------------
message(STATUS "-- Application ------------------------------------------------------")
message(STATUS "App Name:             ${QGC_APP_NAME}")
message(STATUS "Description:          ${QGC_APP_DESCRIPTION}")
message(STATUS "Copyright:            ${QGC_APP_COPYRIGHT}")
message(STATUS "Organization:         ${QGC_ORG_NAME} (${QGC_ORG_DOMAIN})")
message(STATUS "Package name:         ${QGC_PACKAGE_NAME}")
message(STATUS "Settings version:     ${QGC_SETTINGS_VERSION}")
message(STATUS "------------------------------------------------------------------")

# ----------------------------------------------------------------------------
# Option flags
# ----------------------------------------------------------------------------
message(STATUS "-- Build & Feature Flags -------------------------------------------")
OptionOutput("Build shared libraries"                 BUILD_SHARED_LIBS)
OptionOutput("Stable build"                           QGC_STABLE_BUILD)
OptionOutput("Use build caching"                      QGC_USE_CACHE)
OptionOutput("Enable testing"                         QGC_BUILD_TESTING)
OptionOutput("Enable QML debugging"                   QGC_DEBUG_QML)
OptionOutput("Enable Herelink support"                QGC_ENABLE_HERELINK)
OptionOutput("Enable UTM Adapter"                     QGC_UTM_ADAPTER)
OptionOutput("Enable 3D Viewer"                       QGC_VIEWER3D)
OptionOutput("Enable Bluetooth links"                 QGC_ENABLE_BLUETOOTH)
OptionOutput("Enable ZeroConf compatibility"          QGC_ZEROCONF_ENABLED)
OptionOutput("Disable AIRLink"                        QGC_AIRLINK_DISABLED)
OptionOutput("Disable serial links"                   QGC_NO_SERIAL_LINK)
OptionOutput("Enable UVC devices"                     QGC_ENABLE_UVC)
OptionOutput("Enable GStreamer video"                 QGC_ENABLE_GST_VIDEOSTREAMING)
OptionOutput("Enable Qt video backend"                QGC_ENABLE_QT_VIDEOSTREAMING)
OptionOutput("Disable APM MAVLink dialect"            QGC_DISABLE_APM_MAVLINK)
OptionOutput("Disable APM plugin"                     QGC_DISABLE_APM_PLUGIN)
OptionOutput("Disable APM plugin factory"             QGC_DISABLE_APM_PLUGIN_FACTORY)
OptionOutput("Disable PX4 plugin"                     QGC_DISABLE_PX4_PLUGIN)
OptionOutput("Disable PX4 plugin factory"             QGC_DISABLE_PX4_PLUGIN_FACTORY)
message(STATUS "------------------------------------------------------------------")

# ----------------------------------------------------------------------------
# Repository & dependency settings
# ----------------------------------------------------------------------------
message(STATUS "-- External Dependencies -------------------------------------------")
message(STATUS "MAVLink repo URL:     ${QGC_MAVLINK_GIT_REPO}")
message(STATUS "MAVLink repo tag:     ${QGC_MAVLINK_GIT_TAG}")
message(STATUS "CPM cache directory:  ${CPM_SOURCE_CACHE}")
message(STATUS "QML output directory: ${QT_QML_OUTPUT_DIRECTORY}")
message(STATUS "------------------------------------------------------------------")

# ----------------------------------------------------------------------------
# Platform-specific settings
# ----------------------------------------------------------------------------
if(ANDROID)
  message(STATUS "-- Android ---------------------------------------------------------")
  message(STATUS "Target SDK:           ${QGC_QT_ANDROID_TARGET_SDK_VERSION}")
  message(STATUS "Package source dir:   ${QGC_ANDROID_PACKAGE_SOURCE_DIR}")
  message(STATUS "APK signing:          ${QT_ANDROID_SIGN_APK} / AAB signing: ${QT_ANDROID_SIGN_AAB}")
  message(STATUS "Use target build dir: ${QT_USE_TARGET_ANDROID_BUILD_DIR}")
  message(STATUS "NDK host system:      ${ANDROID_NDK_HOST_SYSTEM_NAME}")
  message(STATUS "SDK root:             ${ANDROID_SDK_ROOT}")
  message(STATUS "Deployment type:      ${QT_ANDROID_DEPLOYMENT_TYPE}")
  message(STATUS "------------------------------------------------------------------")
endif()

if(MACOS)
  message(STATUS "-- macOS -----------------------------------------------------------")
  message(STATUS "Bundle ID:            ${QGC_MACOS_BUNDLE_ID}")
  message(STATUS "Info plist path:      ${QGC_MACOS_PLIST_PATH}")
  message(STATUS "Icon directory:       ${QGC_MACOS_ICON_PATH}")
  message(STATUS "Entitlements path:    ${QGC_MACOS_ENTITLEMENTS_PATH}")
  message(STATUS "------------------------------------------------------------------")
endif()

if(WIN32)
  message(STATUS "-- Windows ---------------------------------------------------------")
  message(STATUS "Install header bmp:   ${QGC_WINDOWS_INSTALL_HEADER_PATH}")
  message(STATUS "Icon path:            ${QGC_WINDOWS_ICON_PATH}")
  message(STATUS "RC resource file:     ${QGC_WINDOWS_RESOURCE_FILE_PATH}")
  message(STATUS "------------------------------------------------------------------")
endif()

if(LINUX)
  message(STATUS "-- Linux -----------------------------------------------------------")
  message(STATUS "AppImage icon path:   ${QGC_APPIMAGE_ICON_PATH}")
  message(STATUS "------------------------------------------------------------------")
endif()
