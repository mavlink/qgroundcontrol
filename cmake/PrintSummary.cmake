# ============================================================================
# QGroundControl Build Configuration Summary
# Prints a comprehensive summary of the build configuration
# ============================================================================

# ----------------------------------------------------------------------------
# Configuration Timestamp
# ----------------------------------------------------------------------------
string(TIMESTAMP QGC_CONFIGURE_TIME "%Y-%m-%d %H:%M:%S %Z")
message(STATUS "")
message(STATUS "==================================================================")
message(STATUS "QGroundControl Configuration Summary")
message(STATUS "Generated at: ${QGC_CONFIGURE_TIME}")
message(STATUS "==================================================================")

# ----------------------------------------------------------------------------
# Helper Macro for ON/OFF Options
# ----------------------------------------------------------------------------
macro(OptionOutput _label)
    if(${ARGN})
        set(_val "ON")
    else()
        set(_val "OFF")
    endif()
    message(STATUS "  ${_label}: ${_val}")
endmacro()

# ----------------------------------------------------------------------------
# CMake System Information
# ----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "CMake System:")
message(STATUS "  CMake version:      ${CMAKE_VERSION}")
message(STATUS "  Generator:          ${CMAKE_GENERATOR}")
message(STATUS "  Build type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "  Source directory:   ${CMAKE_SOURCE_DIR}")
message(STATUS "  Install prefix:     ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Host system:        ${CMAKE_HOST_SYSTEM_NAME} ${CMAKE_HOST_SYSTEM_VERSION}")
message(STATUS "  Target system:      ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
if(CMAKE_TOOLCHAIN_FILE)
    message(STATUS "  Toolchain file:     ${CMAKE_TOOLCHAIN_FILE}")
endif()
if(CMAKE_PREFIX_PATH)
    message(STATUS "  Prefix path:        ${CMAKE_PREFIX_PATH}")
endif()

message(STATUS "")
message(STATUS "Compiler & Linker:")
message(STATUS "  C++ compiler:       ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ standard:       C++${CMAKE_CXX_STANDARD}")
if(CMAKE_CXX_FLAGS)
    message(STATUS "  Compiler flags:     ${CMAKE_CXX_FLAGS}")
endif()
if(CMAKE_EXE_LINKER_FLAGS)
    message(STATUS "  Linker flags:       ${CMAKE_EXE_LINKER_FLAGS}")
endif()

# ----------------------------------------------------------------------------
# Application Metadata
# ----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "Application:")
message(STATUS "  Name:               ${QGC_APP_NAME}")
message(STATUS "  Version:            ${QGC_APP_VERSION_STR}")
message(STATUS "  Description:        ${QGC_APP_DESCRIPTION}")
message(STATUS "  Organization:       ${QGC_ORG_NAME} (${QGC_ORG_DOMAIN})")
message(STATUS "  Package name:       ${QGC_PACKAGE_NAME}")
message(STATUS "  Settings version:   ${QGC_SETTINGS_VERSION}")

# ----------------------------------------------------------------------------
# Build & Feature Flags
# ----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "Build & Feature Flags:")
OptionOutput("Stable build                          " QGC_STABLE_BUILD)
OptionOutput("Use build caching                     " QGC_USE_CACHE)
OptionOutput("Enable testing                        " QGC_BUILD_TESTING)
OptionOutput("Enable QML debugging                  " QGC_DEBUG_QML)
OptionOutput("Enable 3D Viewer                      " QGC_VIEWER3D)
OptionOutput("Enable Bluetooth links                " QGC_ENABLE_BLUETOOTH)
OptionOutput("Enable ZeroConf compatibility         " QGC_ZEROCONF_ENABLED)
OptionOutput("Disable AIRLink                       " QGC_AIRLINK_DISABLED)
OptionOutput("Disable serial links                  " QGC_NO_SERIAL_LINK)
OptionOutput("Enable UVC devices                    " QGC_ENABLE_UVC)
OptionOutput("Enable GStreamer video                " QGC_ENABLE_GST_VIDEOSTREAMING)
OptionOutput("Enable Qt video backend               " QGC_ENABLE_QT_VIDEOSTREAMING)
OptionOutput("Disable APM MAVLink dialect           " QGC_DISABLE_APM_MAVLINK)
OptionOutput("Disable APM plugin                    " QGC_DISABLE_APM_PLUGIN)
OptionOutput("Disable PX4 plugin                    " QGC_DISABLE_PX4_PLUGIN)

# ----------------------------------------------------------------------------
# External Dependencies
# ----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "External Dependencies:")
message(STATUS "  MAVLink repo:       ${QGC_MAVLINK_GIT_REPO}")
message(STATUS "  MAVLink tag:        ${QGC_MAVLINK_GIT_TAG}")
message(STATUS "  CPM cache:          ${CPM_SOURCE_CACHE}")
message(STATUS "  QML output dir:     ${QT_QML_OUTPUT_DIRECTORY}")

# ----------------------------------------------------------------------------
# Platform-Specific Settings
# ----------------------------------------------------------------------------
if(ANDROID)
  message(STATUS "")
  message(STATUS "Android Platform:")
  message(STATUS "  Target SDK:         ${QGC_QT_ANDROID_TARGET_SDK_VERSION}")
  message(STATUS "  Min SDK:            ${QGC_QT_ANDROID_MIN_SDK_VERSION}")
  message(STATUS "  Package:            ${QGC_ANDROID_PACKAGE_NAME}")
  message(STATUS "  APK signing:        ${QT_ANDROID_SIGN_APK}")
  message(STATUS "  AAB signing:        ${QT_ANDROID_SIGN_AAB}")
endif()

if(MACOS)
  message(STATUS "")
  message(STATUS "macOS Platform:")
  message(STATUS "  Bundle ID:          ${QGC_MACOS_BUNDLE_ID}")
  message(STATUS "  Deployment target:  ${CMAKE_OSX_DEPLOYMENT_TARGET}")
  if(QGC_MACOS_UNIVERSAL_BUILD)
    message(STATUS "  Architectures:      ${CMAKE_OSX_ARCHITECTURES}")
  endif()
endif()

if(WIN32 AND NOT ANDROID)
  message(STATUS "")
  message(STATUS "Windows Platform:")
  message(STATUS "  Icon:               ${QGC_WINDOWS_ICON_PATH}")
  message(STATUS "  Resource file:      ${QGC_WINDOWS_RESOURCE_FILE_PATH}")
endif()

if(LINUX AND NOT ANDROID)
  message(STATUS "")
  message(STATUS "Linux Platform:")
  if(QGC_CREATE_APPIMAGE)
    message(STATUS "  AppImage:           Enabled")
  endif()
endif()

message(STATUS "")
message(STATUS "==================================================================")
message(STATUS "")
