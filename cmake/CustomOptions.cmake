# ============================================================================
# QGroundControl Build Configuration Options
# All options can be overridden by custom builds via CustomOverrides.cmake
# ============================================================================

include(CMakeDependentOption)

# Load centralized build configuration from .github/build-config.json
include(BuildConfig)

set(QGC_CUSTOM_DIR "custom" CACHE STRING "Custom build overlay directory, relative to the source root")

# ============================================================================
# Application Metadata
# ============================================================================

set(QGC_APP_NAME "QGroundControl" CACHE STRING "Application name")
string(TIMESTAMP _copyright_year "%Y")
set(QGC_APP_COPYRIGHT "Copyright (c) ${_copyright_year} QGroundControl. All rights reserved." CACHE STRING "Copyright notice")
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
option(QGC_UNITY_BUILD "Enable unity builds for faster compilation" OFF)
option(QGC_BUILD_INSTALLER "Build platform installers/packages" ON)
option(QGC_ENABLE_WERROR "Treat compiler warnings as errors for QGC source code" ON)

# Debug-dependent options
# Note: CMAKE_BUILD_TYPE is empty on multi-config generators (VS, Ninja Multi-Config).
# Multi-config generators always get _QGC_DEBUG_BUILD=TRUE because Debug is selected
# at build time, not configure time. Release-only CI jobs should pass
# -DQGC_BUILD_TESTING=OFF explicitly to skip test compilation.
if(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_QGC_DEBUG_BUILD TRUE)
else()
    set(_QGC_DEBUG_BUILD FALSE)
endif()
cmake_dependent_option(QGC_BUILD_TESTING "Enable unit tests" ON "_QGC_DEBUG_BUILD" OFF)
cmake_dependent_option(QGC_DEBUG_QML "Enable QML debugging/profiling" ON "_QGC_DEBUG_BUILD" OFF)
cmake_dependent_option(QGC_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF "_QGC_DEBUG_BUILD" OFF)
cmake_dependent_option(QT_QML_NO_CACHEGEN "Skip qmlcachegen (faster Debug builds, slower QML startup)" ON "_QGC_DEBUG_BUILD" OFF)
option(QGC_ENABLE_CLANG_TIDY "Enable clang-tidy static analysis during build" OFF)
option(QGC_TIME_TRACE "Emit per-TU Clang -ftime-trace JSON for build profiling (Clang only)" OFF)
option(QGC_SPLIT_DWARF "Use -gsplit-dwarf + --gdb-index for faster Debug links (Linux/Android ELF only; marginal win with mold)" OFF)

# Git options
option(GIT_SUBMODULE "Update submodules during configuration" OFF)

# ============================================================================
# Dependency Resolution (system libraries vs CPM downloads)
# ============================================================================
# Lets packagers (FreeBSD Ports, flatpak, snap, Debian) build against system
# libraries instead of fetching sources. These drive CPM's built-in switches:
#   QGC_USE_SYSTEM_LIBS  -> find_package() first, fall back to CPM download.
#   QGC_SYSTEM_LIBS_ONLY -> find_package() only; misses are hard configure errors.
# Per-package FIND_PACKAGE_ARGUMENTS wiring is incremental (geographiclib is the
# reference conversion): unconverted packages error under SYSTEM_LIBS_ONLY, and
# packages needing the CPM source tree (qgc_require_cpm_added sites) fail either way.
option(QGC_USE_SYSTEM_LIBS "Prefer system libraries (find_package) over CPM downloads" OFF)
option(QGC_SYSTEM_LIBS_ONLY "Require system libraries; never download dependencies" OFF)

if(QGC_USE_SYSTEM_LIBS OR QGC_SYSTEM_LIBS_ONLY)
    set(CPM_USE_LOCAL_PACKAGES ON)
endif()
if(QGC_SYSTEM_LIBS_ONLY)
    set(CPM_LOCAL_PACKAGES_ONLY ON)
endif()

# Link parallelism (Ninja only)
set(QGC_LINK_PARALLEL_LEVEL 2 CACHE STRING "Maximum parallel link jobs (prevents OOM during LTO)")
# ---- GStreamer SDK download / debug ----
# Fail closed: only the SDK-download platforms (Android/macOS/iOS/Windows) hit this path;
# Linux uses system pkg-config and never downloads, so ON is a no-op there.
option(GStreamer_REQUIRE_CHECKSUM "Fail if an SDK download's checksum cannot be verified (set OFF to bypass)" ON)
option(GStreamer_DEBUG "Print GStreamer CMake debug messages" OFF)
set(QGC_GST_DOWNLOAD_TIMEOUT "" CACHE STRING "GStreamer SDK download wall-clock timeout (seconds, default 1200)")
set(QGC_GST_DOWNLOAD_INACTIVITY_TIMEOUT "" CACHE STRING "GStreamer SDK download inactivity timeout (seconds, default 60)")

# Coverage thresholds
set(QGC_COVERAGE_LINE_THRESHOLD 30 CACHE STRING "Minimum line coverage percentage")
set(QGC_COVERAGE_BRANCH_THRESHOLD 20 CACHE STRING "Minimum branch coverage percentage")

# Valgrind options
set(QGC_VALGRIND_TIMEOUT_MULTIPLIER 20 CACHE STRING "Timeout multiplier for Valgrind")

# ============================================================================
# Compression Format Options
# ============================================================================
# Core formats (gzip, xz, zstd, zip) are always enabled.
# These optional formats are rarely used in the drone ecosystem.

option(QGC_ENABLE_BZIP2 "Enable BZip2 decompression support" OFF)
option(QGC_ENABLE_LZ4 "Enable LZ4 decompression support" OFF)

# ============================================================================
# Communication Options
# ============================================================================

option(QGC_NO_SERIAL_LINK "Disable serial port communication" OFF)

# ============================================================================
# Video Streaming Options
# ============================================================================

option(QGC_ENABLE_GST_VIDEOSTREAMING "Enable GStreamer video backend" ON)

# ============================================================================
# MAVLink Configuration
# ============================================================================

set(QGC_MAVLINK_GIT_REPO "https://github.com/mavlink/mavlink.git" CACHE STRING "MAVLink repository URL")
set(QGC_MAVLINK_GIT_TAG "c409cf690454db6d3e004bd14173bc6c7ff1e0ff" CACHE STRING "MAVLink repository commit/tag")
set(QGC_MAVLINK_DIALECT "all" CACHE STRING "MAVLink dialect")
set(QGC_MAVLINK_VERSION "2.0" CACHE STRING "MAVLink protocol version")

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
set(QGC_QT_ANDROID_COMPILE_SDK_VERSION "${QGC_CONFIG_ANDROID_PLATFORM}" CACHE STRING "Android compile SDK version")
set(QGC_QT_ANDROID_TARGET_SDK_VERSION "${QGC_CONFIG_ANDROID_PLATFORM}" CACHE STRING "Android target SDK version")
set(QGC_QT_ANDROID_MIN_SDK_VERSION "${QGC_CONFIG_ANDROID_MIN_SDK}" CACHE STRING "Android minimum SDK version")
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
# iOS Platform
# ----------------------------------------------------------------------------
set(QGC_IOS_DEPLOYMENT_TARGET "${QGC_CONFIG_IOS_DEPLOYMENT_TARGET}" CACHE STRING "iOS minimum deployment target")
set(QGC_IOS_TARGETED_DEVICE_FAMILY "1,2" CACHE STRING "iOS targeted device family (1=iPhone, 2=iPad)")

# ----------------------------------------------------------------------------
# Linux Platform
# ----------------------------------------------------------------------------
# Distro-aware defaults for native (non-Docker) builds. Docker builds pass these
# explicitly via -D (see deploy/docker/entrypoint.sh), which overrides the cache.
include(LinuxDistro)

# Fedora/Arch glibc exceeds the AppImage floor (appimagelint noise there); native
# package generator follows the distro (DEB/RPM via CPack, Arch via makepkg).
set(_qgc_appimagelint_default ON)
set(_qgc_cpack_default "")
if(QGC_LINUX_DISTRO_FAMILY STREQUAL "debian")
    set(_qgc_cpack_default "DEB")
elseif(QGC_LINUX_DISTRO_FAMILY STREQUAL "rhel")
    set(_qgc_appimagelint_default OFF)
    set(_qgc_cpack_default "RPM")
elseif(QGC_LINUX_DISTRO_FAMILY STREQUAL "arch")
    set(_qgc_appimagelint_default OFF)
endif()

option(QGC_CREATE_APPIMAGE "Create AppImage package after build" ON)
option(QGC_RUN_APPIMAGELINT "Run the appimagelint distro-compatibility check after AppImage creation" ${_qgc_appimagelint_default})
set(QGC_APPIMAGE_ICON_256_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/QGroundControl_256.png" CACHE FILEPATH "AppImage 256x256 icon path")
set(QGC_APPIMAGE_ICON_SCALABLE_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/QGroundControl.svg" CACHE FILEPATH "AppImage SVG icon path")
set(QGC_APPIMAGE_APPRUN_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/AppRun" CACHE FILEPATH "AppImage AppRun script path")
set(QGC_APPIMAGE_DESKTOP_ENTRY_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.desktop.in" CACHE FILEPATH "AppImage desktop entry path")
set(QGC_APPIMAGE_METADATA_PATH "${CMAKE_SOURCE_DIR}/deploy/linux/org.mavlink.qgroundcontrol.appdata.xml.in" CACHE FILEPATH "AppImage metadata path")
set(QGC_APPIMAGE_APPDATA_DEVELOPER "qgroundcontrol" CACHE STRING "AppImage developer name")
# Optional CPack native package built by the `qgc-package` target, alongside the
# platform's default installer (AppImage / NSIS .exe / DMG). Empty = installer only.
# WIN32/APPLE (not MACOS/LINUX) because Toolchain.cmake — which sets those — is
# included after this file.
if(WIN32)
    set(_qgc_cpack_strings "" "NSIS" "IFW" "TXZ")
elseif(APPLE)
    set(_qgc_cpack_strings "" "DragNDrop" "Bundle" "productbuild" "IFW" "TXZ")
else()
    set(_qgc_cpack_strings "" "DEB" "RPM" "TXZ")
endif()
set(QGC_CPACK_GENERATOR "${_qgc_cpack_default}" CACHE STRING "Optional CPack generator for the qgc-package target (alongside the default installer)")
set_property(CACHE QGC_CPACK_GENERATOR PROPERTY STRINGS ${_qgc_cpack_strings})

# ----------------------------------------------------------------------------
# Windows Platform
# ----------------------------------------------------------------------------
set(QGC_WINDOWS_INSTALL_HEADER_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/installheader.bmp" CACHE FILEPATH "Windows installer header image")
set(QGC_WINDOWS_ICON_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/WindowsQGC.ico" CACHE FILEPATH "Windows application icon")
set(QGC_WINDOWS_RESOURCE_FILE_PATH "${CMAKE_SOURCE_DIR}/deploy/windows/QGroundControl.rc" CACHE FILEPATH "Windows resource file")

# ============================================================================
# Qt Configuration
# ============================================================================

set(QGC_QT_MINIMUM_VERSION "${QGC_CONFIG_QT_MINIMUM_VERSION}" CACHE STRING "Minimum supported Qt version")
set(QGC_QT_MAXIMUM_VERSION "${QGC_CONFIG_QT_VERSION}" CACHE STRING "Maximum supported Qt version")

set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml" CACHE PATH "QML output directory")
set(QML_IMPORT_PATH "${QT_QML_OUTPUT_DIRECTORY}" CACHE STRING "Additional QML import paths")

option(QT_SILENCE_MISSING_DEPENDENCY_TARGET_WARNING "Silence missing dependency warnings" OFF)
option(QT_ENABLE_VERBOSE_DEPLOYMENT "Enable verbose deployment output" OFF)
option(QT_DEBUG_FIND_PACKAGE "Print search paths when package not found" ON)
# qmlls.ini writes into the source tree dirty the CI checkout.
if(DEFINED ENV{CI})
    set(_qmlls_ini_default OFF)
else()
    set(_qmlls_ini_default ON)
endif()
option(QT_QML_GENERATE_QMLLS_INI "Generate qmlls.ini for QML language server" ${_qmlls_ini_default})
unset(_qmlls_ini_default)
option(QT_QMLLINT_CONTEXT_PROPERTY_DUMP "Emit qmllint context property data (Qt 6.11+; no-op on older)" ON)
option(QT_QML_GENERATE_QMLLINT "Run qmllint at build time" OFF)

set(QGC_QT_DISABLE_DEPRECATED_UP_TO "0x060B00" CACHE STRING "Disable Qt APIs deprecated before this version")
set(QGC_QT_ENABLE_STRICT_MODE_UP_TO "0x060B00" CACHE STRING "Enable strict Qt API mode up to this version")

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
