# ============================================================================
# QGroundControl Build Configuration Options
# All options can be overridden by custom builds via CustomOverrides.cmake
# ============================================================================

include(CMakeDependentOption)

# ============================================================================
# Application Metadata
# ============================================================================

set(QGC_APP_NAME "QGroundControl" CACHE STRING "Application name")
set(QGC_APP_COPYRIGHT "Copyright (c) 2025 QGroundControl. All rights reserved." CACHE STRING "Copyright notice")
set(QGC_APP_DESCRIPTION "Open Source Ground Control App" CACHE STRING "Application description")
set(QGC_ORG_NAME "QGroundControl" CACHE STRING "Organization name")
set(QGC_ORG_DOMAIN "qgroundcontrol.com" CACHE STRING "Organization domain")
set(QGC_PACKAGE_NAME "org.mavlink.qgroundcontrol" CACHE STRING "Package identifier")

# Settings version - increment to clear stored settings on next boot after incompatible changes
set(QGC_SETTINGS_VERSION "9" CACHE STRING "Settings schema version")

# ============================================================================
# Build Configuration
# ============================================================================

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(QGC_STABLE_BUILD "Stable release build (disables daily build features)" OFF)
option(QGC_USE_CACHE "Enable compiler caching (ccache/sccache)" ON)
option(QGC_BUILD_INSTALLER "Build platform installers/packages" ON)

# Debug-dependent options
cmake_dependent_option(QGC_BUILD_TESTING "Enable unit tests" ON "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)
cmake_dependent_option(QGC_DEBUG_QML "Enable QML debugging/profiling" ON "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)

# ============================================================================
# Feature Flags
# ============================================================================

option(QGC_UTM_ADAPTER "Enable UTM (Unmanned Traffic Management) Adapter" OFF)
option(QGC_VIEWER3D "Enable 3D Viewer (requires Qt Quick 3D)" ON)

# MAVLink Inspector is disabled by default due to GPL licensing of QtCharts
# option(QGC_DISABLE_MAVLINK_INSPECTOR "Disable MAVLink Inspector" OFF)

# ============================================================================
# Communication Options
# ============================================================================

option(QGC_ENABLE_BLUETOOTH "Enable Bluetooth communication links" ON)
option(QGC_ZEROCONF_ENABLED "Enable ZeroConf/Bonjour discovery" OFF)
option(QGC_AIRLINK_DISABLED "Disable AIRLink support" ON)
option(QGC_NO_SERIAL_LINK "Disable serial port communication" OFF)

# ============================================================================
# Video Streaming Options
# ============================================================================

option(QGC_ENABLE_UVC "Enable UVC (USB Video Class) device support" ON)
option(QGC_ENABLE_GST_VIDEOSTREAMING "Enable GStreamer video backend" ON)
cmake_dependent_option(QGC_CUSTOM_GST_PACKAGE "Use QGC-provided GStreamer packages" OFF "QGC_ENABLE_GST_VIDEOSTREAMING" OFF)
option(QGC_ENABLE_QT_VIDEOSTREAMING "Enable QtMultimedia video backend" OFF)

# ============================================================================
# Joystick/Input Configuration
# ============================================================================

# Example custom SDL game controller mapping:
# set(SDL_GAMECONTROLLERCONFIG "0300000009120000544f000011010000,OpenTX Radiomaster TX16S,leftx:a3,lefty:a2,rightx:a0,righty:a1,platform:Linux" CACHE STRING "Custom SDL mappings")

# ============================================================================
# MAVLink Configuration
# ============================================================================

set(QGC_MAVLINK_GIT_REPO "https://github.com/mavlink/c_library_v2.git" CACHE STRING "MAVLink repository URL")
set(QGC_MAVLINK_GIT_TAG "a9a10b52a6c87e54676fea22d2936c1b8b733f99" CACHE STRING "MAVLink repository commit/tag")

# ============================================================================
# Autopilot Plugin Configuration
# ============================================================================

# ArduPilot (APM) Plugin
option(QGC_DISABLE_APM_MAVLINK "Disable ArduPilot MAVLink dialect" OFF)
option(QGC_DISABLE_APM_PLUGIN "Disable ArduPilot plugin" OFF)
option(QGC_DISABLE_APM_PLUGIN_FACTORY "Disable ArduPilot plugin factory" OFF)

# PX4 Plugin
option(QGC_DISABLE_PX4_PLUGIN "Disable PX4 plugin" OFF)
option(QGC_DISABLE_PX4_PLUGIN_FACTORY "Disable PX4 plugin factory" OFF)

# ============================================================================
# Platform-Specific Configuration
# ============================================================================

# ----------------------------------------------------------------------------
# Android Platform
# ----------------------------------------------------------------------------
set(QGC_QT_ANDROID_COMPILE_SDK_VERSION "35" CACHE STRING "Android compile SDK version")
set(QGC_QT_ANDROID_TARGET_SDK_VERSION "35" CACHE STRING "Android target SDK version")
set(QGC_QT_ANDROID_MIN_SDK_VERSION "28" CACHE STRING "Android minimum SDK version")
set(QGC_ANDROID_PACKAGE_NAME "${QGC_PACKAGE_NAME}" CACHE STRING "Android package identifier")
set(QGC_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/android" CACHE PATH "Android package source directory")
set(QT_ANDROID_DEPLOYMENT_TYPE "" CACHE STRING "Android deployment type (empty or Release)")
option(QT_ANDROID_SIGN_APK "Enable APK signing" OFF)
option(QT_ANDROID_SIGN_AAB "Enable AAB signing" OFF)
option(QT_USE_TARGET_ANDROID_BUILD_DIR "Use target-specific Android build directory" OFF)

# ----------------------------------------------------------------------------
# macOS Platform
# ----------------------------------------------------------------------------
set(QGC_MACOS_PLIST_PATH "${CMAKE_SOURCE_DIR}/deploy/macos/MacOSXBundleInfo.plist.in" CACHE FILEPATH "macOS Info.plist template path")
set(QGC_MACOS_BUNDLE_ID "${QGC_PACKAGE_NAME}" CACHE STRING "macOS bundle identifier")
set(QGC_MACOS_ICON_PATH "${CMAKE_SOURCE_DIR}/deploy/macos/qgroundcontrol.icns" CACHE FILEPATH "macOS application icon path")
set(QGC_MACOS_ENTITLEMENTS_PATH "${CMAKE_SOURCE_DIR}/deploy/macos/qgroundcontrol.entitlements" CACHE FILEPATH "macOS entitlements file path")
option(QGC_MACOS_UNIVERSAL_BUILD "Build macOS universal binary (x86_64h + arm64)" ON)

# ----------------------------------------------------------------------------
# Linux Platform
# ----------------------------------------------------------------------------
option(QGC_CREATE_APPIMAGE "Create AppImage package after build" ON)
set(QGC_APPIMAGE_ICON_256_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/QGroundControl_256.png" CACHE FILEPATH "AppImage 256x256 icon path")
set(QGC_APPIMAGE_ICON_SCALABLE_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/QGroundControl.svg" CACHE FILEPATH "AppImage SVG icon path")
set(QGC_APPIMAGE_APPRUN_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/AppRun" CACHE FILEPATH "AppImage AppRun script path")
set(QGC_APPIMAGE_DESKTOP_ENTRY_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.desktop.in" CACHE FILEPATH "AppImage desktop entry path")
set(QGC_APPIMAGE_METADATA_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.appdata.xml.in" CACHE FILEPATH "AppImage metadata path")
set(QGC_APPIMAGE_APPDATA_DEVELOPER "qgroundcontrol" CACHE STRING "AppImage developer name")

# ----------------------------------------------------------------------------
# Windows Platform
# ----------------------------------------------------------------------------
set(QGC_WINDOWS_INSTALL_HEADER_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/installheader.bmp" CACHE FILEPATH "Windows installer header image")
set(QGC_WINDOWS_ICON_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/WindowsQGC.ico" CACHE FILEPATH "Windows application icon")
set(QGC_WINDOWS_RESOURCE_FILE_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/QGroundControl.rc" CACHE FILEPATH "Windows resource file")

# ============================================================================
# Qt Configuration
# ============================================================================

set(QGC_QT_MINIMUM_VERSION "6.10.0" CACHE STRING "Minimum supported Qt version")
set(QGC_QT_MAXIMUM_VERSION "6.10.0" CACHE STRING "Maximum supported Qt version")

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH "QML output directory")
set(QML_IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}" CACHE STRING "Additional QML import paths")

option(QT_SILENCE_MISSING_DEPENDENCY_TARGET_WARNING "Silence missing dependency warnings" OFF)
option(QT_ENABLE_VERBOSE_DEPLOYMENT "Enable verbose deployment output" OFF)
option(QT_DEBUG_FIND_PACKAGE "Print search paths when package not found" ON)
option(QT_QML_GENERATE_QMLLS_INI "Generate qmlls.ini for QML language server" ON)

# Debug environment variables (uncomment to enable)
# set(ENV{QT_DEBUG_PLUGINS} "1")
# set(ENV{QML_IMPORT_TRACE} "1")

# ============================================================================
# CMake Package Manager (CPM)
# ============================================================================

# Uncomment to use named cache directories for better organization
# set(CPM_USE_NAMED_CACHE_DIRECTORIES ON CACHE BOOL "Use package name subdirectories in CPM cache")

# ============================================================================
# CMake Configuration
# ============================================================================

# Uncomment for verbose package finding
# option(CMAKE_FIND_DEBUG_MODE "Print search paths when finding packages" OFF)
