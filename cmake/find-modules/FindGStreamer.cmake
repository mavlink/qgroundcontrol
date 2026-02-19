# ============================================================================
# FindGStreamer.cmake
# CMake find module for GStreamer multimedia framework
#
# Handles multiple platforms: Windows, Linux, macOS, Android, iOS
# Supports both static and dynamic linking
# ============================================================================

# ----------------------------------------------------------------------------
# Default Configuration
# ----------------------------------------------------------------------------

# Set default version based on platform (from build-config.json)
if(NOT DEFINED GStreamer_FIND_VERSION)
    if(LINUX)
        set(GStreamer_FIND_VERSION 1.20)
    elseif(WIN32)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_WIN_VERSION})
    elseif(ANDROID)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_ANDROID_VERSION})
    elseif(MACOS OR IOS)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_MACOS_VERSION})
    else()
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_VERSION})
    endif()
endif()

# Determine GStreamer root directory from various environment variables
if(NOT DEFINED GStreamer_ROOT_DIR)
    if(DEFINED GSTREAMER_ROOT)
        set(GStreamer_ROOT_DIR ${GSTREAMER_ROOT})
    elseif(DEFINED GStreamer_ROOT)
        set(GStreamer_ROOT_DIR ${GStreamer_ROOT})
    endif()

    if(DEFINED GStreamer_ROOT_DIR AND NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(STATUS "GStreamer: User-provided directory does not exist: ${GStreamer_ROOT_DIR}")
    endif()
endif()

# Static vs dynamic linking preference
if(NOT DEFINED GStreamer_USE_STATIC_LIBS)
    if(ANDROID OR IOS)
        set(GStreamer_USE_STATIC_LIBS ON)
    else()
        set(GStreamer_USE_STATIC_LIBS OFF)
    endif()
endif()

# Framework usage (macOS/iOS only)
if(NOT DEFINED GStreamer_USE_FRAMEWORK)
    if(APPLE)
        set(GStreamer_USE_FRAMEWORK ON)
    else()
        set(GStreamer_USE_FRAMEWORK OFF)
    endif()
endif()

# ============================================================================
# Platform-Specific Configuration
# ============================================================================

set(PKG_CONFIG_ARGN)

# ----------------------------------------------------------------------------
# Windows Platform
# ----------------------------------------------------------------------------
if(WIN32)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(DEFINED ENV{GSTREAMER_1_0_ROOT_X86_64} AND EXISTS "$ENV{GSTREAMER_1_0_ROOT_X86_64}")
            set(GStreamer_ROOT_DIR "$ENV{GSTREAMER_1_0_ROOT_X86_64}")
        elseif(MSVC AND DEFINED ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64} AND EXISTS "$ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}")
            set(GStreamer_ROOT_DIR "$ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64}")
        elseif(MINGW AND DEFINED ENV{GSTREAMER_1_0_ROOT_MINGW_X86_64} AND EXISTS "$ENV{GSTREAMER_1_0_ROOT_MINGW_X86_64}")
            set(GStreamer_ROOT_DIR "$ENV{GSTREAMER_1_0_ROOT_MINGW_X86_64}")
        elseif(EXISTS "C:/Program Files/gstreamer/1.0/msvc_x86_64")
            set(GStreamer_ROOT_DIR "C:/Program Files/gstreamer/1.0/msvc_x86_64")
        elseif(EXISTS "C:/gstreamer/1.0/msvc_x86_64")
            set(GStreamer_ROOT_DIR "C:/gstreamer/1.0/msvc_x86_64")
        endif()
    endif()

    cmake_path(CONVERT "${GStreamer_ROOT_DIR}" TO_CMAKE_PATH_LIST GStreamer_ROOT_DIR NORMALIZE)
    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(FATAL_ERROR "Could not locate GStreamer - check installation or set environment/cmake variables")
    endif()

    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")

    set(PKG_CONFIG_EXECUTABLE "${GStreamer_ROOT_DIR}/bin/pkg-config.exe")
    set(ENV{PKG_CONFIG} "${PKG_CONFIG_EXECUTABLE}")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig;$ENV{PKG_CONFIG_PATH}")
    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )

# ----------------------------------------------------------------------------
# Linux Platform
# ----------------------------------------------------------------------------
elseif(LINUX)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(EXISTS "/usr")
            set(GStreamer_ROOT_DIR "/usr")
        endif()
    endif()

    cmake_path(CONVERT "${GStreamer_ROOT_DIR}" TO_CMAKE_PATH_LIST GStreamer_ROOT_DIR NORMALIZE)
    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(FATAL_ERROR "Could not locate GStreamer - check installation or set environment/cmake variables")
    endif()

    if((EXISTS "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu" ) AND (EXISTS "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu/gstreamer-1.0"))
        set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
    elseif(EXISTS "${GStreamer_ROOT_DIR}/lib")
        set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    else()
        message(FATAL_ERROR "Could not locate GStreamer - check installation or set environment/cmake variables")
    endif()

    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")

    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")

# ----------------------------------------------------------------------------
# Android Platform
# ----------------------------------------------------------------------------
elseif(ANDROID)
    if(QGC_CUSTOM_GST_PACKAGE)
        set(_gst_android_url "https://qgroundcontrol.s3.us-west-2.amazonaws.com/android-gstreamer/qgc-android-gstreamer-${GStreamer_FIND_VERSION}.tar.xz")
    else()
        set(_gst_android_url "https://gstreamer.freedesktop.org/data/pkg/android/${GStreamer_FIND_VERSION}/gstreamer-1.0-android-universal-${GStreamer_FIND_VERSION}.tar.xz")
        # https://gstreamer.freedesktop.org/data/pkg/android/${GStreamer_FIND_VERSION}/gstreamer-1.0-android-universal-${GStreamer_FIND_VERSION}.tar.xz.sha256sum
        # set(_gst_android_url_hash "be92cf477d140c270b480bd8ba0e26b1e01c8db042c46b9e234d87352112e485")
    endif()

    set(_gst_android_cache_root "${CMAKE_BINARY_DIR}/_deps/gstreamer-android")
    set(_gst_android_archive "${_gst_android_cache_root}/gstreamer-android-${GStreamer_FIND_VERSION}.tar.xz")
    set(_gst_android_extract_root "${_gst_android_cache_root}/src")
    set(_gst_android_expected_dir "${_gst_android_extract_root}/gstreamer-1.0-android-universal-${GStreamer_FIND_VERSION}")

    if(NOT EXISTS "${_gst_android_expected_dir}" AND NOT EXISTS "${_gst_android_extract_root}/arm64")
        file(MAKE_DIRECTORY "${_gst_android_cache_root}")
        file(MAKE_DIRECTORY "${_gst_android_extract_root}")

        set(_gst_download_ok FALSE)
        foreach(_attempt RANGE 1 3)
            file(DOWNLOAD "${_gst_android_url}" "${_gst_android_archive}"
                STATUS _dl_status
                LOG _dl_log
                SHOW_PROGRESS
                TLS_VERIFY ON
            )
            list(GET _dl_status 0 _dl_code)
            if(_dl_code EQUAL 0)
                set(_gst_download_ok TRUE)
                break()
            endif()

            if(_attempt LESS 3)
                message(WARNING "GStreamer Android download failed (attempt ${_attempt}/3): ${_dl_status}. Retrying...")
                execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 2)
            endif()
        endforeach()

        if(NOT _gst_download_ok)
            message(FATAL_ERROR "Failed to download GStreamer Android package from ${_gst_android_url}: ${_dl_status}\n${_dl_log}")
        endif()

        file(ARCHIVE_EXTRACT
            INPUT "${_gst_android_archive}"
            DESTINATION "${_gst_android_extract_root}"
        )
    endif()

    if(EXISTS "${_gst_android_expected_dir}")
        set(gstreamer_SOURCE_DIR "${_gst_android_expected_dir}")
    elseif(EXISTS "${_gst_android_extract_root}/arm64")
        # Some archives may extract directly to ABI folders without a top-level directory.
        set(gstreamer_SOURCE_DIR "${_gst_android_extract_root}")
    else()
        message(FATAL_ERROR "Could not locate extracted GStreamer Android package under ${_gst_android_extract_root}")
    endif()

    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
            set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/armv7")
        elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
            set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/arm64")
        elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
            set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/x86")
        elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
            set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/x86_64")
        endif()
    endif()

    cmake_path(CONVERT "${GStreamer_ROOT_DIR}" TO_CMAKE_PATH_LIST GStreamer_ROOT_DIR NORMALIZE)
    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(FATAL_ERROR "Could not locate GStreamer - check installation or set environment/cmake variables")
    endif()

    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")

    set(ENV{PKG_CONFIG_PATH} "")
    if(CMAKE_HOST_WIN32)
        set(PKG_CONFIG_EXECUTABLE "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/tools/windows/pkg-config.exe")
        set(ENV{PKG_CONFIG} "${PKG_CONFIG_EXECUTABLE}")
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig")
        list(APPEND PKG_CONFIG_ARGN --dont-define-prefix)
    elseif(CMAKE_HOST_UNIX)
        if(CMAKE_HOST_APPLE)
            # Try to find pkg-config in common Homebrew locations and in PATH
            find_program(PKG_CONFIG_EXECUTABLE
                NAMES pkg-config
                PATHS /opt/homebrew/bin /usr/local/bin
                NO_DEFAULT_PATH
            )
            if(NOT PKG_CONFIG_EXECUTABLE)
                # Fallback to PATH search if not found in Homebrew locations
                find_program(PKG_CONFIG_EXECUTABLE pkg-config)
            endif()
            if(NOT PKG_CONFIG_EXECUTABLE)
                message(FATAL_ERROR "Could not find pkg-config. Please install pkg-config using tools/setup/install_dependencies.py --platform macos.")
            endif()
        endif()
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    endif()
    list(APPEND PKG_CONFIG_ARGN
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )

# ----------------------------------------------------------------------------
# macOS Platform
# ----------------------------------------------------------------------------
elseif(MACOS)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(EXISTS "/Library/Frameworks/GStreamer.framework")
            set(GStreamer_ROOT_DIR "/Library/Frameworks/GStreamer.framework/Versions/1.0")
        elseif(EXISTS "/opt/homebrew/opt/gstreamer")
            set(GStreamer_ROOT_DIR "/opt/homebrew/opt/gstreamer")
        elseif(EXISTS "/usr/local/opt/gstreamer")
            set(GStreamer_ROOT_DIR "/usr/local/opt/gstreamer")
        endif()
    endif()

    cmake_path(CONVERT "${GStreamer_ROOT_DIR}" TO_CMAKE_PATH_LIST GStreamer_ROOT_DIR NORMALIZE)
    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(FATAL_ERROR "Could not locate GStreamer - check installation or set environment/cmake variables")
    endif()

    if(GStreamer_USE_FRAMEWORK)
        set(GSTREAMER_FRAMEWORK_PATH "${GStreamer_ROOT_DIR}/../..")
    endif()

    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")

    set(PKG_CONFIG_EXECUTABLE "${GStreamer_ROOT_DIR}/bin/pkg-config")
    set(ENV{PKG_CONFIG} "${PKG_CONFIG_EXECUTABLE}")
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )

# ----------------------------------------------------------------------------
# iOS Platform (Currently Unsupported)
# ----------------------------------------------------------------------------
elseif(IOS)
    message(FATAL_ERROR "GStreamer for iOS is currently unsupported")

    CPMAddPackage(
        NAME gstreamer
        VERSION ${GStreamer_FIND_VERSION}
        URL "https://gstreamer.freedesktop.org/data/pkg/ios/${GStreamer_FIND_VERSION}/gstreamer-1.0-devel-${GStreamer_FIND_VERSION}-ios-universal.pkg"
    )

    set(GST_PKG_FILE "${gstreamer_SOURCE_DIR}/gstreamer.pkg")
    set(GST_EXPAND_DIR "${gstreamer_SOURCE_DIR}/gstreamer-pkg-expanded")
    set(GST_PAYLOAD_DIR "${GST_EXPAND_DIR}/Payload")

    file(MAKE_DIRECTORY "${GST_EXPAND_DIR}")
    execute_process(
        COMMAND pkgutil --expand-full "${GST_PKG_FILE}" "${GST_EXPAND_DIR}"
        RESULT_VARIABLE _pkgutil_rc
    )
    if(NOT _pkgutil_rc EQUAL 0)
        message(FATAL_ERROR "pkgutil failed to expand GStreamer .pkg")
    endif()

    execute_process(
        COMMAND xar -xf "${GST_EXPAND_DIR}/gstreamer-1.0-devel-${GStreamer_FIND_VERSION}-ios-universal.pkg/Payload"
                --directory "${GST_PAYLOAD_DIR}"
        RESULT_VARIABLE _xar_rc
    )
    if(NOT _xar_rc EQUAL 0)
        message(FATAL_ERROR "xar failed to extract GStreamer Payload")
    endif()

    # set(GSTREAMER_FRAMEWORK_PATH "/Library/Developer/GStreamer/iPhone.sdk" CACHE PATH "Path of GStreamer.Framework")
    set(GSTREAMER_FRAMEWORK_PATH "${GST_PAYLOAD_DIR}/usr/local/Frameworks/GStreamer.framework")

    set(GStreamer_ROOT_DIR "${GSTREAMER_FRAMEWORK_PATH}/Versions/1.0")

    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(FATAL_ERROR "Could not locate GStreamer - check installation or set environment/cmake variables")
    endif()

    set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_FRAMEWORK_PATH}/Headers")
endif()

# ----------------------------------------------------------------------------
# Validation
# ----------------------------------------------------------------------------
if(NOT EXISTS "${GStreamer_ROOT_DIR}" OR NOT EXISTS "${GSTREAMER_LIB_PATH}" OR NOT EXISTS "${GSTREAMER_PLUGIN_PATH}" OR NOT EXISTS "${GSTREAMER_INCLUDE_PATH}")
    message(FATAL_ERROR "GStreamer: Could not locate required directories - check installation or set GStreamer_ROOT_DIR")
endif()

if(GStreamer_USE_FRAMEWORK AND NOT EXISTS "${GSTREAMER_FRAMEWORK_PATH}")
    message(FATAL_ERROR "GStreamer: Could not locate framework at ${GSTREAMER_FRAMEWORK_PATH}")
endif()

# ============================================================================
# Plugin & Dependency Configuration
# ============================================================================

if(GStreamer_USE_STATIC_LIBS)
    set(GSTREAMER_EXTRA_DEPS
        gstreamer-base-1.0
        gstreamer-video-1.0
        gstreamer-gl-1.0
        gstreamer-gl-prototypes-1.0
        gstreamer-rtsp-1.0
        # gstreamer-gl-egl-1.0
        # gstreamer-gl-wayland-1.0
        # gstreamer-gl-x11-1.0
    )

    set(GSTREAMER_PLUGINS
        coreelements
        dav1d
        isomp4
        libav
        matroska
        mpegtsdemux
        opengl
        openh264
        playback
        rtp
        rtpmanager
        rtsp
        sdpelem
        tcp
        typefindfunctions
        udp
        videoparsersbad
        vpx
    )
    if(ANDROID)
        list(APPEND GSTREAMER_PLUGINS androidmedia) # vulkan
    elseif(APPLE)
        list(APPEND GSTREAMER_PLUGINS applemedia vulkan)
    elseif(WIN32)
        list(APPEND GSTREAMER_PLUGINS d3d d3d11 d3d12 dxva nvcodec)
    elseif(LINUX)
        list(APPEND GSTREAMER_PLUGINS nvcodec qsv va vulkan) # qml6 - GStreamer provided qml6 is xcb only
    endif()
endif()

if(ANDROID)
    set(GStreamer_Mobile_MODULE_NAME gstreamer_android)
    set(G_IO_MODULES openssl)
    set(G_IO_MODULES_PATH "${GStreamer_ROOT_DIR}/lib/gio/modules")

    set(GStreamer_NDK_BUILD_PATH  "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/")
    set(GSTREAMER_ANDROID_MODULE_NAME gstreamer_android)
    set(GSTREAMER_JAVA_SRC_DIR "${CMAKE_BINARY_DIR}/android-build-${CMAKE_PROJECT_NAME}/src")
    set(GSTREAMER_ASSETS_DIR "${CMAKE_BINARY_DIR}/android-build-${CMAKE_PROJECT_NAME}/assets")

    configure_file(
        "${GStreamer_NDK_BUILD_PATH}/gstreamer_android-1.0.c.in"
        "${GStreamer_Mobile_MODULE_NAME}.c"
    )
endif()

# ============================================================================
# Package Discovery
# ============================================================================

if(GStreamer_USE_FRAMEWORK)
    list(APPEND CMAKE_FRAMEWORK_PATH "${GSTREAMER_FRAMEWORK_PATH}")
endif()

if(GStreamer_USE_STATIC_LIBS)
    list(APPEND PKG_CONFIG_ARGN "--static")
endif()

find_package(PkgConfig REQUIRED QUIET)

list(PREPEND CMAKE_PREFIX_PATH ${GStreamer_ROOT_DIR})
if(LINUX)
    pkg_check_modules(PC_GSTREAMER REQUIRED gstreamer-1.0>=${GStreamer_FIND_VERSION})
else()
    pkg_check_modules(PC_GSTREAMER REQUIRED gstreamer-1.0=${GStreamer_FIND_VERSION})
endif()
set(GStreamer_VERSION "${PC_GSTREAMER_VERSION}")

# ============================================================================
# Component Discovery
# ============================================================================

# Helper function to find individual GStreamer components
function(find_gstreamer_component component pkgconfig_name)
    set(target GStreamer::${component})

    if(NOT TARGET ${target})
        string(TOUPPER ${component} upper)
        pkg_check_modules(PC_GSTREAMER_${upper} IMPORTED_TARGET ${pkgconfig_name})
        if(TARGET PkgConfig::PC_GSTREAMER_${upper})
            qt_add_library(GStreamer::${component} INTERFACE IMPORTED)
            target_link_libraries(GStreamer::${component} INTERFACE PkgConfig::PC_GSTREAMER_${upper})
            if("PC_GSTREAMER_${upper}" MATCHES "PC_GSTREAMER_GL")
                get_target_property(_qt_incs PkgConfig::PC_GSTREAMER_GL INTERFACE_INCLUDE_DIRECTORIES)
                set(__qt_fixed_incs)
                foreach(path IN LISTS _qt_incs)
                    if(IS_DIRECTORY "${path}")
                        list(APPEND __qt_fixed_incs "${path}")
                    endif()
                endforeach()
                set_property(TARGET PkgConfig::PC_GSTREAMER_GL PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${__qt_fixed_incs}")
            endif()
        endif()
    endif()

    if(TARGET ${target})
        set(GStreamer_${component}_FOUND TRUE PARENT_SCOPE)
    endif()
endfunction()

# ----------------------------------------------------------------------------
# Find Core Components (Always Required)
# ----------------------------------------------------------------------------
find_gstreamer_component(Core gstreamer-1.0)
find_gstreamer_component(Base gstreamer-base-1.0)
find_gstreamer_component(Video gstreamer-video-1.0)
find_gstreamer_component(Gl gstreamer-gl-1.0)
find_gstreamer_component(GlPrototypes gstreamer-gl-prototypes-1.0)
find_gstreamer_component(Rtsp gstreamer-rtsp-1.0)

# ----------------------------------------------------------------------------
# Find Optional Components (Based on FIND_COMPONENTS)
# ----------------------------------------------------------------------------
if(GlEgl IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlEgl gstreamer-gl-egl-1.0)
endif()

if(GlWayland IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlWayland gstreamer-gl-wayland-1.0)
endif()

if(GlX11 IN_LIST GStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlX11 gstreamer-gl-x11-1.0)
endif()

# ============================================================================
# Package Finalization
# ============================================================================

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GStreamer
    VERSION_VAR GStreamer_VERSION
    HANDLE_COMPONENTS
)

# ----------------------------------------------------------------------------
# Create Main GStreamer Target
# ----------------------------------------------------------------------------
if(GStreamer_FOUND AND NOT TARGET GStreamer::GStreamer)
    qt_add_library(GStreamer::GStreamer INTERFACE IMPORTED)

    if(GStreamer_USE_STATIC_LIBS)
        target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
    endif()

    if(APPLE AND GStreamer_USE_FRAMEWORK)
        set(CMAKE_FIND_FRAMEWORK ONLY)
        find_library(GSTREAMER_FRAMEWORK GStreamer
            PATHS
                "${GSTREAMER_FRAMEWORK_PATH}"
                "/Library/Frameworks"
                "/usr/local/opt/gstreamer"
                "/opt/homebrew/opt/gstreamer"
        )
        unset(CMAKE_FIND_FRAMEWORK)
        if(GSTREAMER_FRAMEWORK)
            target_link_libraries(GStreamer::GStreamer INTERFACE ${GSTREAMER_FRAMEWORK})
            target_include_directories(GStreamer::GStreamer INTERFACE "${GSTREAMER_FRAMEWORK}/Headers")
            if(MACOS)
                target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_MACOS_FRAMEWORK)
            endif()
            return()
        else()
            message(FATAL_ERROR "GStreamer: Could not locate GStreamer.framework")
        endif()

    # Android-specific link options
    elseif(ANDROID)
        target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-Bsymbolic")
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-z,notext")
        endif()
    endif()

    target_link_directories(GStreamer::GStreamer INTERFACE ${GSTREAMER_LIB_PATH})

    target_link_libraries(GStreamer::GStreamer
        INTERFACE
            GStreamer::Core
            GStreamer::Base
            GStreamer::Video
            GStreamer::Gl
            GStreamer::GlPrototypes
            GStreamer::Rtsp
    )

    foreach(component IN LISTS GStreamer_FIND_COMPONENTS)
        if(GStreamer_${component}_FOUND)
            target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::${component})
        endif()
    endforeach()

# ----------------------------------------------------------------------------
# Static Plugin Linking (Static Builds Only)
# ----------------------------------------------------------------------------
    if(GStreamer_USE_STATIC_LIBS)
        qt_add_library(GStreamer::Plugins INTERFACE IMPORTED)
        target_link_directories(GStreamer::Plugins INTERFACE ${GSTREAMER_PLUGIN_PATH})

        foreach(plugin IN LISTS GSTREAMER_PLUGINS)
            pkg_check_modules(GST_PLUGIN_${plugin} QUIET IMPORTED_TARGET gst${plugin})
            if(GST_PLUGIN_${plugin}_FOUND)
                target_link_libraries(GStreamer::Plugins INTERFACE PkgConfig::GST_PLUGIN_${plugin})
            else()
                find_library(GST_PLUGIN_${plugin}_LIBRARY
                    NAMES gst${plugin}
                    PATHS
                        ${GSTREAMER_LIB_PATH}
                        ${GSTREAMER_PLUGIN_PATH}
                )
                if(GST_PLUGIN_${plugin}_LIBRARY)
                    target_link_libraries(GStreamer::Plugins INTERFACE ${GST_PLUGIN_${plugin}_LIBRARY})
                    set(GST_PLUGIN_${plugin}_FOUND TRUE)
                endif()
            endif()
            if(GST_PLUGIN_${plugin}_FOUND)
                target_compile_definitions(GStreamer::Plugins INTERFACE GST_PLUGIN_${plugin}_FOUND)
            endif()
        endforeach()

        target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::Plugins)
    endif()
endif()
