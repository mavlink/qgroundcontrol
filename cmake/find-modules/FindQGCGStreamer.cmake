if(NOT DEFINED GStreamer_FIND_VERSION)
    if(LINUX)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_MIN_VERSION})
    elseif(WIN32)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_WIN_VERSION})
    elseif(ANDROID)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_ANDROID_VERSION})
    elseif(IOS)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_IOS_VERSION})
    elseif(MACOS)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_MACOS_VERSION})
    else()
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_VERSION})
    endif()
endif()

if(NOT GStreamer_FIND_VERSION)
    message(FATAL_ERROR "GStreamer version not configured. Ensure BuildConfig.cmake has been included "
        "and .github/build-config.json contains the appropriate gstreamer_*_version entry.")
endif()

if(NOT DEFINED GStreamer_ROOT_DIR)
    if(DEFINED GSTREAMER_ROOT)
        set(GStreamer_ROOT_DIR ${GSTREAMER_ROOT})
    elseif(DEFINED GStreamer_ROOT)
        set(GStreamer_ROOT_DIR ${GStreamer_ROOT})
    endif()

    if(DEFINED GStreamer_ROOT_DIR AND NOT EXISTS "${GStreamer_ROOT_DIR}")
        message(FATAL_ERROR "GStreamer: User-provided directory does not exist: ${GStreamer_ROOT_DIR}\n"
            "Correct the path or unset GStreamer_ROOT_DIR.")
    endif()
endif()

if(NOT DEFINED GStreamer_USE_STATIC_LIBS)
    if(ANDROID OR IOS)
        set(GStreamer_USE_STATIC_LIBS ON)
    else()
        set(GStreamer_USE_STATIC_LIBS OFF)
    endif()
endif()

if(NOT DEFINED GStreamer_USE_FRAMEWORK)
    if(APPLE)
        set(GStreamer_USE_FRAMEWORK ON)
    else()
        set(GStreamer_USE_FRAMEWORK OFF)
    endif()
endif()

include(GStreamerHelpers)

set(PKG_CONFIG_ARGN)

function(_qgc_find_apple_pkg_config OUT_VAR)
    find_program(_qgc_pkg_config
        NAMES pkg-config pkgconf
        PATHS /opt/homebrew/bin /usr/local/bin
        NO_DEFAULT_PATH
    )
    if(NOT _qgc_pkg_config)
        find_program(_qgc_pkg_config NAMES pkg-config pkgconf)
    endif()
    if(NOT _qgc_pkg_config)
        message(FATAL_ERROR
            "Could not find pkg-config.\n"
            "Install dependencies with: python3 tools/setup/install_dependencies.py --platform macos\n"
            "or install pkg-config manually (for example: brew install pkg-config).")
    endif()
    set(${OUT_VAR} "${_qgc_pkg_config}" CACHE FILEPATH "pkg-config executable" FORCE)
    unset(_qgc_pkg_config CACHE)
endfunction()

function(_qgc_validate_expanded_pkg EXPANDED_DIR LABEL)
    file(GLOB _payloads "${EXPANDED_DIR}/*.pkg/Payload")
    if(NOT _payloads)
        file(REMOVE_RECURSE "${EXPANDED_DIR}")
        message(FATAL_ERROR
            "pkgutil expanded GStreamer ${LABEL} package but no payloads were found in ${EXPANDED_DIR}")
    endif()
endfunction()

if(WIN32 AND NOT ANDROID)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
        set(_gst_win_arch "arm64")
    else()
        set(_gst_win_arch "x86_64")
    endif()

    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(_gst_win_arch STREQUAL "arm64")
            if(DEFINED ENV{GSTREAMER_1_0_ROOT_ARM64} AND EXISTS "$ENV{GSTREAMER_1_0_ROOT_ARM64}")
                set(GStreamer_ROOT_DIR "$ENV{GSTREAMER_1_0_ROOT_ARM64}")
            elseif(MSVC AND DEFINED ENV{GSTREAMER_1_0_ROOT_MSVC_ARM64} AND EXISTS "$ENV{GSTREAMER_1_0_ROOT_MSVC_ARM64}")
                set(GStreamer_ROOT_DIR "$ENV{GSTREAMER_1_0_ROOT_MSVC_ARM64}")
            elseif(EXISTS "C:/Program Files/gstreamer/1.0/msvc_arm64")
                set(GStreamer_ROOT_DIR "C:/Program Files/gstreamer/1.0/msvc_arm64")
            elseif(EXISTS "C:/gstreamer/1.0/msvc_arm64")
                set(GStreamer_ROOT_DIR "C:/gstreamer/1.0/msvc_arm64")
            endif()
        else()
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
    endif()

    _gst_normalize_and_validate_root()

    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")

    set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/bin/pkg-config.exe")
    set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}" CACHE FILEPATH "pkg-config executable" FORCE)
    set(ENV{PKG_CONFIG_PATH} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )

elseif(LINUX AND NOT ANDROID)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        set(GStreamer_ROOT_DIR "/usr")
    endif()

    _gst_normalize_and_validate_root()

    if((EXISTS "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu") AND (EXISTS "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu/gstreamer-1.0"))
        set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-gnu")
    elseif((EXISTS "${GStreamer_ROOT_DIR}/lib64") AND (EXISTS "${GStreamer_ROOT_DIR}/lib64/gstreamer-1.0"))
        set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib64")
    elseif((EXISTS "${GStreamer_ROOT_DIR}/lib") AND (EXISTS "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0"))
        set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    else()
        message(FATAL_ERROR "Could not locate GStreamer libraries - check installation or set environment/cmake variables")
    endif()

    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")

    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")

elseif(ANDROID)
    gstreamer_get_package_url(android ${GStreamer_FIND_VERSION} _gst_android_url)
    gstreamer_get_s3_mirror_url(android ${GStreamer_FIND_VERSION} _gst_android_s3_url)
    gstreamer_fetch_checksum(android ${GStreamer_FIND_VERSION} _gst_android_hash)

    if(CPM_SOURCE_CACHE)
        set(_gst_android_cache "${CPM_SOURCE_CACHE}/gstreamer-android")
    else()
        set(_gst_android_cache "${CMAKE_BINARY_DIR}/_deps/gstreamer-android")
    endif()
    gstreamer_resilient_download(
        URLS "${_gst_android_url}" "${_gst_android_s3_url}"
        FILENAME "gstreamer-android-${GStreamer_FIND_VERSION}.tar.xz"
        DESTINATION_DIR "${_gst_android_cache}"
        RESULT_VAR _gst_android_archive
        EXPECTED_HASH "${_gst_android_hash}"
    )

    CPMAddPackage(
        NAME gstreamer
        VERSION ${GStreamer_FIND_VERSION}
        URL "file://${_gst_android_archive}"
    )

    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(CMAKE_ANDROID_ARCH_ABI STREQUAL "armeabi-v7a")
            set(GStreamer_ABI_DIR "armv7")
        elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "arm64-v8a")
            set(GStreamer_ABI_DIR "arm64")
        elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86")
            set(GStreamer_ABI_DIR "x86")
        elseif(CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
            set(GStreamer_ABI_DIR "x86_64")
        else()
            message(FATAL_ERROR "Unsupported Android ABI: ${CMAKE_ANDROID_ARCH_ABI}")
        endif()
        set(GStreamer_ROOT_DIR "${gstreamer_SOURCE_DIR}/${GStreamer_ABI_DIR}")
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    _gst_normalize_and_validate_root()

    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")

    set(ENV{PKG_CONFIG_PATH} "")
    if(CMAKE_HOST_WIN32)
        set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/tools/windows/pkg-config.exe")
        set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}" CACHE FILEPATH "pkg-config executable" FORCE)
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig")
        list(APPEND PKG_CONFIG_ARGN --dont-define-prefix)
    elseif(CMAKE_HOST_UNIX)
        if(CMAKE_HOST_APPLE)
            _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        endif()
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    endif()
    list(APPEND PKG_CONFIG_ARGN
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )

elseif(MACOS AND NOT IOS)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(EXISTS "/Library/Frameworks/GStreamer.framework")
            set(GStreamer_ROOT_DIR "/Library/Frameworks/GStreamer.framework/Versions/1.0")
        elseif(EXISTS "/opt/homebrew/opt/gstreamer")
            set(GStreamer_ROOT_DIR "/opt/homebrew/opt/gstreamer")
            set(GStreamer_USE_FRAMEWORK OFF)
        elseif(EXISTS "/usr/local/opt/gstreamer")
            set(GStreamer_ROOT_DIR "/usr/local/opt/gstreamer")
            set(GStreamer_USE_FRAMEWORK OFF)
        endif()
    endif()

    _gst_normalize_and_validate_root()

    if(GStreamer_USE_FRAMEWORK AND NOT DEFINED GSTREAMER_FRAMEWORK_PATH)
        set(GSTREAMER_FRAMEWORK_PATH "${GStreamer_ROOT_DIR}/../..")
        cmake_path(NORMAL_PATH GSTREAMER_FRAMEWORK_PATH)
    endif()

    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")

    if(GStreamer_USE_FRAMEWORK)
        set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/bin/pkg-config")
        set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}" CACHE FILEPATH "pkg-config executable" FORCE)
        set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    else()
        _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        set(ENV{PKG_CONFIG_PATH} "")
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    endif()
    list(APPEND PKG_CONFIG_ARGN
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )

elseif(IOS)
    if(NOT CMAKE_HOST_APPLE)
        message(FATAL_ERROR "GStreamer for iOS can only be built on macOS")
    endif()

    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(EXISTS "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
            set(GSTREAMER_FRAMEWORK_PATH "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
            set(GStreamer_ROOT_DIR "${GSTREAMER_FRAMEWORK_PATH}/Versions/1.0")
        endif()
    endif()

    if(NOT DEFINED GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
        gstreamer_get_package_url(ios ${GStreamer_FIND_VERSION} _gst_ios_url)
        gstreamer_get_s3_mirror_url(ios ${GStreamer_FIND_VERSION} _gst_ios_s3_url)
        gstreamer_fetch_checksum(ios ${GStreamer_FIND_VERSION} _gst_ios_hash)

        set(_gst_ios_cache_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-ios-${GStreamer_FIND_VERSION}")
        set(_gst_ios_expanded "${_gst_ios_cache_dir}/expanded")

        gstreamer_resilient_download(
            URLS "${_gst_ios_url}" "${_gst_ios_s3_url}"
            FILENAME "gstreamer-ios.pkg"
            DESTINATION_DIR "${_gst_ios_cache_dir}"
            RESULT_VAR _gst_ios_pkg
            EXPECTED_HASH "${_gst_ios_hash}"
        )

        if(EXISTS "${_gst_ios_expanded}")
            set(_gst_ios_complete FALSE)
            file(GLOB _existing_pkg_dirs "${_gst_ios_expanded}/*.pkg")
            foreach(_dir IN LISTS _existing_pkg_dirs)
                if(EXISTS "${_dir}/Payload/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
                    set(_gst_ios_complete TRUE)
                    break()
                endif()
            endforeach()
            if(NOT _gst_ios_complete)
                message(STATUS "GStreamer: cached iOS expansion is incomplete; re-expanding")
                file(REMOVE_RECURSE "${_gst_ios_expanded}")
            endif()
        endif()

        if(NOT EXISTS "${_gst_ios_expanded}")
            message(STATUS "Expanding GStreamer iOS package...")
            execute_process(
                COMMAND pkgutil --expand-full "${_gst_ios_pkg}" "${_gst_ios_expanded}"
                RESULT_VARIABLE _pkgutil_rc
            )
            if(NOT _pkgutil_rc EQUAL 0)
                file(REMOVE_RECURSE "${_gst_ios_expanded}")
                message(FATAL_ERROR
                    "pkgutil failed to expand GStreamer iOS .pkg (exit code: ${_pkgutil_rc})")
            endif()
            _qgc_validate_expanded_pkg("${_gst_ios_expanded}" "iOS")
        endif()

        file(GLOB _pkg_dirs "${_gst_ios_expanded}/*.pkg")
        foreach(_pkg_dir IN LISTS _pkg_dirs)
            if(EXISTS "${_pkg_dir}/Payload/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
                set(GSTREAMER_FRAMEWORK_PATH "${_pkg_dir}/Payload/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
                break()
            endif()
        endforeach()

        if(NOT GSTREAMER_FRAMEWORK_PATH)
            message(FATAL_ERROR "Could not locate GStreamer.framework in downloaded iOS SDK")
        endif()

        set(GStreamer_ROOT_DIR "${GSTREAMER_FRAMEWORK_PATH}/Versions/1.0")
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    _gst_normalize_and_validate_root()

    set(GStreamer_USE_FRAMEWORK ON)
    if(NOT DEFINED GSTREAMER_FRAMEWORK_PATH)
        set(GSTREAMER_FRAMEWORK_PATH "${GStreamer_ROOT_DIR}/../..")
        cmake_path(NORMAL_PATH GSTREAMER_FRAMEWORK_PATH)
    endif()
    set(GSTREAMER_INCLUDE_PATH "${GSTREAMER_FRAMEWORK_PATH}/Headers")
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")

    _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)

    set(ENV{PKG_CONFIG_PATH} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    list(APPEND PKG_CONFIG_ARGN
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )
endif()

if(NOT EXISTS "${GStreamer_ROOT_DIR}" OR NOT EXISTS "${GSTREAMER_LIB_PATH}" OR NOT EXISTS "${GSTREAMER_PLUGIN_PATH}" OR NOT EXISTS "${GSTREAMER_INCLUDE_PATH}")
    message(FATAL_ERROR "GStreamer: Could not locate required directories - check installation or set GStreamer_ROOT_DIR")
endif()

if(GStreamer_USE_FRAMEWORK AND NOT EXISTS "${GSTREAMER_FRAMEWORK_PATH}")
    message(FATAL_ERROR "GStreamer: Could not locate framework at ${GSTREAMER_FRAMEWORK_PATH}")
endif()

# Sub-libraries used directly by QGC source code. Plugin transitive deps
# are resolved automatically from each plugin's .pc file.
if(NOT DEFINED GSTREAMER_EXTRA_DEPS)
    set(GSTREAMER_EXTRA_DEPS
        gstreamer-base-1.0
        gstreamer-gl-1.0
        gstreamer-gl-prototypes-1.0
        gstreamer-rtsp-1.0
        gstreamer-video-1.0
    )
    if(WIN32)
        list(APPEND GSTREAMER_EXTRA_DEPS graphene-1.0)
    endif()
    if(ANDROID OR IOS)
        list(APPEND GSTREAMER_EXTRA_DEPS gio-2.0)
    endif()
    if(ANDROID)
        list(APPEND GSTREAMER_EXTRA_DEPS gmodule-2.0 zlib)
    endif()
endif()

if(NOT DEFINED GSTREAMER_PLUGINS)
    set(GSTREAMER_PLUGINS
        coreelements
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
        videoconvertscale
        videoparsersbad
        vpx
    )
    if(ANDROID)
        list(APPEND GSTREAMER_PLUGINS androidmedia dav1d)
    elseif(APPLE)
        list(APPEND GSTREAMER_PLUGINS applemedia dav1d vulkan)
    elseif(WIN32)
        list(APPEND GSTREAMER_PLUGINS d3d d3d11 d3d12 dav1d dxva nvcodec)
    elseif(LINUX)
        list(APPEND GSTREAMER_PLUGINS dav1d nvcodec qsv va vulkan)
    endif()
endif()

if(ANDROID)
    set(GStreamer_Mobile_MODULE_NAME gstreamer_android)
    set(G_IO_MODULES openssl)
    set(G_IO_MODULES_PATH "${GStreamer_ROOT_DIR}/lib/gio/modules")
    set(GStreamer_NDK_BUILD_PATH "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build")
    set(GStreamer_JAVA_SRC_DIR "${CMAKE_BINARY_DIR}/android-build-${CMAKE_PROJECT_NAME}/src")
    set(GStreamer_ASSETS_DIR "${CMAKE_BINARY_DIR}/android-build-${CMAKE_PROJECT_NAME}/assets")
elseif(IOS)
    set(GStreamer_Mobile_MODULE_NAME gstreamer_mobile)
    set(G_IO_MODULES openssl)
    set(G_IO_MODULES_PATH "${GStreamer_ROOT_DIR}/lib/gio/modules")
    set(GStreamer_ASSETS_DIR "${CMAKE_BINARY_DIR}/assets")
endif()

set(GStreamer_ROOT_DIR "${GStreamer_ROOT_DIR}" CACHE PATH "GStreamer SDK root directory" FORCE)

if(GStreamer_USE_FRAMEWORK)
    list(APPEND CMAKE_FRAMEWORK_PATH "${GSTREAMER_FRAMEWORK_PATH}")
endif()

if(GStreamer_USE_STATIC_LIBS)
    list(APPEND PKG_CONFIG_ARGN "--static")
endif()

find_package(PkgConfig REQUIRED QUIET)

list(PREPEND CMAKE_PREFIX_PATH ${GStreamer_ROOT_DIR})
if(LINUX)
    pkg_check_modules(PC_GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0>=${GStreamer_FIND_VERSION})
else()
    pkg_check_modules(PC_GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0=${GStreamer_FIND_VERSION})
endif()
set(GStreamer_VERSION "${PC_GSTREAMER_VERSION}")

function(find_gstreamer_component component pkgconfig_name)
    set(target GStreamer::${component})

    if(NOT TARGET ${target})
        string(TOUPPER ${component} upper)
        pkg_check_modules(PC_GSTREAMER_${upper} IMPORTED_TARGET ${pkgconfig_name})
        if(TARGET PkgConfig::PC_GSTREAMER_${upper})
            qt_add_library(GStreamer::${component} INTERFACE IMPORTED)
            target_link_libraries(GStreamer::${component} INTERFACE PkgConfig::PC_GSTREAMER_${upper})
            if(upper STREQUAL "GL")
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
        set(QGCGStreamer_${component}_FOUND TRUE PARENT_SCOPE)
        set(GStreamer_${component}_FOUND TRUE PARENT_SCOPE)
    endif()
endfunction()

find_gstreamer_component(Core gstreamer-1.0)
find_gstreamer_component(Base gstreamer-base-1.0)
find_gstreamer_component(Video gstreamer-video-1.0)
find_gstreamer_component(Gl gstreamer-gl-1.0)
find_gstreamer_component(GlPrototypes gstreamer-gl-prototypes-1.0)
find_gstreamer_component(Rtsp gstreamer-rtsp-1.0)

if(GlEgl IN_LIST QGCGStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlEgl gstreamer-gl-egl-1.0)
endif()

if(GlWayland IN_LIST QGCGStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlWayland gstreamer-gl-wayland-1.0)
endif()

if(GlX11 IN_LIST QGCGStreamer_FIND_COMPONENTS)
    find_gstreamer_component(GlX11 gstreamer-gl-x11-1.0)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QGCGStreamer
    REQUIRED_VARS GStreamer_ROOT_DIR GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH
    VERSION_VAR GStreamer_VERSION
    HANDLE_COMPONENTS
)

if(QGCGStreamer_FOUND AND NOT TARGET GStreamer::GStreamer)
    qt_add_library(GStreamer::GStreamer INTERFACE IMPORTED)

    if(GStreamer_USE_STATIC_LIBS)
        target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
    endif()

    if(APPLE AND GStreamer_USE_FRAMEWORK)
        set(_saved_find_framework "${CMAKE_FIND_FRAMEWORK}")
        set(CMAKE_FIND_FRAMEWORK ONLY)
        cmake_path(GET GSTREAMER_FRAMEWORK_PATH PARENT_PATH _gst_framework_parent)
        find_library(GSTREAMER_FRAMEWORK GStreamer
            PATHS
                "${_gst_framework_parent}"
                "${GSTREAMER_FRAMEWORK_PATH}"
                "/Library/Frameworks"
                "/usr/local/opt/gstreamer"
                "/opt/homebrew/opt/gstreamer"
        )
        set(CMAKE_FIND_FRAMEWORK "${_saved_find_framework}")
        if(GSTREAMER_FRAMEWORK)
            target_link_libraries(GStreamer::GStreamer INTERFACE ${GSTREAMER_FRAMEWORK})
            target_include_directories(GStreamer::GStreamer INTERFACE "${GSTREAMER_FRAMEWORK}/Headers")
            if(MACOS)
                target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_MACOS_FRAMEWORK)
            endif()
        else()
            message(FATAL_ERROR "GStreamer: Could not locate GStreamer.framework")
        endif()

    elseif(ANDROID)
        target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-Bsymbolic")
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-z,notext")
        endif()
    endif()

    target_link_directories(GStreamer::GStreamer INTERFACE ${GSTREAMER_LIB_PATH})

    # On macOS with framework, the umbrella library already provides all component
    # symbols. Linking individual component dylibs would add @rpath/libgst*.dylib
    # load commands that cannot be resolved inside the app bundle.
    if(NOT (MACOS AND GStreamer_USE_FRAMEWORK AND GSTREAMER_FRAMEWORK))
        target_link_libraries(GStreamer::GStreamer
            INTERFACE
                GStreamer::Core
                GStreamer::Base
                GStreamer::Video
                GStreamer::Gl
                GStreamer::GlPrototypes
                GStreamer::Rtsp
        )

        foreach(component IN LISTS QGCGStreamer_FIND_COMPONENTS)
            if(GStreamer_${component}_FOUND)
                target_link_libraries(GStreamer::GStreamer INTERFACE GStreamer::${component})
            endif()
        endforeach()
    endif()

    if(GStreamer_USE_STATIC_LIBS)
        qt_add_library(GStreamer::Plugins INTERFACE IMPORTED)
        target_link_directories(GStreamer::Plugins INTERFACE ${GSTREAMER_PLUGIN_PATH})

        foreach(plugin IN LISTS GSTREAMER_PLUGINS)
            pkg_check_modules(GST_PLUGIN_${plugin} QUIET IMPORTED_TARGET gst${plugin})
            if(GST_PLUGIN_${plugin}_FOUND)
                if(TARGET PkgConfig::GST_PLUGIN_${plugin})
                    target_link_libraries(GStreamer::Plugins INTERFACE PkgConfig::GST_PLUGIN_${plugin})
                else()
                    target_link_libraries(GStreamer::Plugins INTERFACE ${GST_PLUGIN_${plugin}_STATIC_LIBRARIES})
                    target_link_directories(GStreamer::Plugins INTERFACE ${GST_PLUGIN_${plugin}_STATIC_LIBRARY_DIRS})
                endif()
            else()
                find_library(GST_PLUGIN_${plugin}_LIBRARY
                    NAMES gst${plugin}
                    PATHS
                        ${GSTREAMER_LIB_PATH}
                        ${GSTREAMER_PLUGIN_PATH}
                    NO_DEFAULT_PATH
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
