include(GStreamerHelpers)

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
            "Correct the path or unset GStreamer_ROOT_DIR to allow auto-download.")
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

function(_qgc_windows_sdk_complete ROOT_DIR OUT_VAR)
    set(_required_paths
        "bin/pkg-config.exe"
        "include/gstreamer-1.0"
        "lib/gstreamer-1.0"
        "lib/pkgconfig/gstreamer-1.0.pc"
    )

    set(_is_complete TRUE)
    foreach(_path IN LISTS _required_paths)
        if(NOT EXISTS "${ROOT_DIR}/${_path}")
            set(_is_complete FALSE)
            break()
        endif()
    endforeach()

    set(${OUT_VAR} "${_is_complete}" PARENT_SCOPE)
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
        set(_gst_win_platform "windows_msvc_arm64")
    else()
        set(_gst_win_arch "x86_64")
        set(_gst_win_platform "windows_msvc_x64")
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

    if(DEFINED GStreamer_ROOT_DIR AND EXISTS "${GStreamer_ROOT_DIR}")
        _qgc_windows_sdk_complete("${GStreamer_ROOT_DIR}" _gst_win_sdk_complete)
        if(NOT _gst_win_sdk_complete)
            message(WARNING
                "Existing GStreamer SDK at ${GStreamer_ROOT_DIR} is incomplete "
                "(missing devel/runtime artifacts). Falling back to auto-download.")
            unset(GStreamer_ROOT_DIR)
        endif()
    endif()

    if(NOT DEFINED GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
        if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
            message(FATAL_ERROR
                "Automatic GStreamer download on Windows requires MSVC 64-bit (x64 or ARM64).\n"
                "Set GStreamer_ROOT_DIR to a complete SDK for this toolchain/architecture.")
        endif()

        set(_gst_win_cache_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-win-${GStreamer_FIND_VERSION}")
        set(_gst_win_extracted "${_gst_win_cache_dir}/sdk")

        gstreamer_download_sdk(${_gst_win_platform} ${GStreamer_FIND_VERSION}
            "gstreamer-${_gst_win_arch}.exe" "${_gst_win_cache_dir}" _gst_win_exe)

        set(_gst_win_root "")
        if(EXISTS "${_gst_win_extracted}")
            if(EXISTS "${_gst_win_extracted}/bin/pkg-config.exe")
                set(_gst_win_root "${_gst_win_extracted}")
            else()
                file(GLOB_RECURSE _pkg_config_files "${_gst_win_extracted}/pkg-config.exe")
                if(_pkg_config_files)
                    list(GET _pkg_config_files 0 _first_pkg_config)
                    cmake_path(GET _first_pkg_config PARENT_PATH _bin_dir)
                    cmake_path(GET _bin_dir PARENT_PATH _gst_win_root)
                endif()
            endif()
        endif()

        if(NOT _gst_win_root
           OR NOT EXISTS "${_gst_win_root}/lib/gstreamer-1.0"
           OR NOT EXISTS "${_gst_win_root}/lib/pkgconfig/gstreamer-1.0.pc")
            file(REMOVE_RECURSE "${_gst_win_extracted}")
            cmake_path(NATIVE_PATH _gst_win_exe _gst_win_exe_native)
            cmake_path(NATIVE_PATH _gst_win_extracted _gst_win_extracted_native)
            set(_gst_win_installer_log "${_gst_win_cache_dir}/installer-${_gst_win_arch}.log")
            cmake_path(NATIVE_PATH _gst_win_installer_log _gst_win_installer_log_native)

            message(STATUS "Installing GStreamer ${_gst_win_arch} SDK (silent)...")
            execute_process(
                COMMAND "${_gst_win_exe_native}"
                    /VERYSILENT /SUPPRESSMSGBOXES /SP- /NORESTART
                    "/LOG=${_gst_win_installer_log_native}"
                    "/DIR=${_gst_win_extracted_native}"
                RESULT_VARIABLE _installer_rc
                TIMEOUT 600
            )
            if(NOT _installer_rc EQUAL 0)
                file(REMOVE_RECURSE "${_gst_win_extracted}")
                message(FATAL_ERROR "GStreamer installer failed (exit code: ${_installer_rc}). "
                    "See installer log: ${_gst_win_installer_log}")
            endif()

            set(_gst_win_root "${_gst_win_extracted}")
            if(NOT EXISTS "${_gst_win_root}/bin/pkg-config.exe")
                file(GLOB_RECURSE _pkg_config_files "${_gst_win_extracted}/pkg-config.exe")
                if(_pkg_config_files)
                    list(GET _pkg_config_files 0 _first_pkg_config)
                    cmake_path(GET _first_pkg_config PARENT_PATH _bin_dir)
                    cmake_path(GET _bin_dir PARENT_PATH _gst_win_root)
                    message(STATUS "GStreamer: SDK root at ${_gst_win_root}")
                endif()
            endif()

            if(NOT EXISTS "${_gst_win_root}/bin/pkg-config.exe"
               OR NOT EXISTS "${_gst_win_root}/lib/gstreamer-1.0"
               OR NOT EXISTS "${_gst_win_root}/lib/pkgconfig/gstreamer-1.0.pc")
                file(REMOVE_RECURSE "${_gst_win_extracted}")
                message(FATAL_ERROR "GStreamer SDK extracted but required files are missing.\n"
                    "Delete ${_gst_win_cache_dir} and re-run cmake to retry.")
            endif()
        endif()

        set(GStreamer_ROOT_DIR "${_gst_win_root}")
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    _gst_normalize_and_validate_root()
    _gst_set_standard_paths()

    set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/bin/pkg-config.exe")
    set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}" CACHE FILEPATH "pkg-config executable" FORCE)
    set(ENV{PKG_CONFIG_PATH} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    list(APPEND PKG_CONFIG_ARGN --dont-define-prefix)

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
    if(CPM_SOURCE_CACHE)
        set(_gst_android_cache "${CPM_SOURCE_CACHE}/gstreamer-android")
    else()
        set(_gst_android_cache "${CMAKE_BINARY_DIR}/_deps/gstreamer-android")
    endif()
    gstreamer_download_sdk(android ${GStreamer_FIND_VERSION}
        "gstreamer-android-${GStreamer_FIND_VERSION}.tar.xz" "${_gst_android_cache}" _gst_android_archive)

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
    _gst_set_standard_paths()

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

    if(NOT DEFINED GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
        set(_gst_mac_cache_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-mac-${GStreamer_FIND_VERSION}")
        set(_gst_mac_expanded "${_gst_mac_cache_dir}/expanded")
        set(_gst_mac_devel_expanded "${_gst_mac_cache_dir}/expanded-devel")
        set(_gst_mac_root "${_gst_mac_cache_dir}/root")
        set(_gst_mac_required_plugin_dir "${_gst_mac_root}/lib/gstreamer-1.0")
        set(_gst_mac_required_include_dir "${_gst_mac_root}/include/gstreamer-1.0")
        set(_gst_mac_required_pc_file "${_gst_mac_root}/lib/pkgconfig/gstreamer-1.0.pc")

        gstreamer_download_sdk(macos ${GStreamer_FIND_VERSION}
            "gstreamer.pkg" "${_gst_mac_cache_dir}" _gst_mac_pkg)
        gstreamer_download_sdk(macos_devel ${GStreamer_FIND_VERSION}
            "gstreamer-devel.pkg" "${_gst_mac_cache_dir}" _gst_mac_devel_pkg)

        if(EXISTS "${_gst_mac_root}/.merge_complete")
            if(NOT EXISTS "${_gst_mac_required_plugin_dir}"
               OR NOT EXISTS "${_gst_mac_required_include_dir}"
               OR NOT EXISTS "${_gst_mac_required_pc_file}")
                message(STATUS "GStreamer: cached macOS SDK is incomplete; rebuilding cache")
                file(REMOVE_RECURSE "${_gst_mac_root}" "${_gst_mac_expanded}" "${_gst_mac_devel_expanded}")
            endif()
        endif()

        if(NOT EXISTS "${_gst_mac_root}/.merge_complete")
            file(REMOVE_RECURSE "${_gst_mac_root}")
            file(REMOVE_RECURSE "${_gst_mac_expanded}" "${_gst_mac_devel_expanded}")

            message(STATUS "Expanding GStreamer macOS runtime package...")
            execute_process(
                COMMAND pkgutil --expand-full "${_gst_mac_pkg}" "${_gst_mac_expanded}"
                RESULT_VARIABLE _pkgutil_rc
            )
            if(NOT _pkgutil_rc EQUAL 0)
                file(REMOVE_RECURSE "${_gst_mac_expanded}")
                message(FATAL_ERROR "pkgutil failed to expand GStreamer runtime .pkg (exit code: ${_pkgutil_rc})")
            endif()
            _qgc_validate_expanded_pkg("${_gst_mac_expanded}" "runtime")

            message(STATUS "Expanding GStreamer macOS devel package...")
            execute_process(
                COMMAND pkgutil --expand-full "${_gst_mac_devel_pkg}" "${_gst_mac_devel_expanded}"
                RESULT_VARIABLE _pkgutil_rc
            )
            if(NOT _pkgutil_rc EQUAL 0)
                file(REMOVE_RECURSE "${_gst_mac_devel_expanded}")
                message(FATAL_ERROR "pkgutil failed to expand GStreamer devel .pkg (exit code: ${_pkgutil_rc})")
            endif()
            _qgc_validate_expanded_pkg("${_gst_mac_devel_expanded}" "devel")

            file(MAKE_DIRECTORY "${_gst_mac_root}")
            foreach(_expanded_dir IN ITEMS "${_gst_mac_expanded}" "${_gst_mac_devel_expanded}")
                file(GLOB _sub_pkg_dirs "${_expanded_dir}/*.pkg")
                foreach(_pkg_dir IN LISTS _sub_pkg_dirs)
                    if(EXISTS "${_pkg_dir}/Payload")
                        file(GLOB _payload_entries "${_pkg_dir}/Payload/*")
                        foreach(_entry IN LISTS _payload_entries)
                            cmake_path(GET _entry FILENAME _entry_name)
                            if(_entry_name STREQUAL "Headers")
                                continue()
                            endif()
                            cmake_path(IS_PREFIX _gst_mac_root "${_gst_mac_root}/${_entry_name}" NORMALIZE _is_safe)
                            if(NOT _is_safe)
                                message(FATAL_ERROR "GStreamer: Path traversal detected in extracted SDK entry: ${_entry_name}")
                            endif()
                            file(COPY "${_entry}" DESTINATION "${_gst_mac_root}")
                        endforeach()
                    endif()
                endforeach()
            endforeach()
            file(TOUCH "${_gst_mac_root}/.merge_complete")
        endif()

        if(NOT EXISTS "${_gst_mac_required_plugin_dir}"
           OR NOT EXISTS "${_gst_mac_required_include_dir}"
           OR NOT EXISTS "${_gst_mac_required_pc_file}")
            file(REMOVE_RECURSE "${_gst_mac_root}" "${_gst_mac_expanded}" "${_gst_mac_devel_expanded}")
            message(FATAL_ERROR "Downloaded macOS GStreamer SDK is incomplete "
                "(required runtime/devel artifacts were not found).\n"
                "Install manually from https://gstreamer.freedesktop.org/download/ or set GStreamer_ROOT_DIR.")
        endif()

        set(GStreamer_ROOT_DIR "${_gst_mac_root}")
        set(GStreamer_USE_FRAMEWORK OFF)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    _gst_normalize_and_validate_root()
    _gst_set_standard_paths()

    if(GStreamer_USE_FRAMEWORK AND NOT DEFINED GSTREAMER_FRAMEWORK_PATH)
        set(GSTREAMER_FRAMEWORK_PATH "${GStreamer_ROOT_DIR}/../..")
        cmake_path(NORMAL_PATH GSTREAMER_FRAMEWORK_PATH)
    endif()

    if(GStreamer_USE_FRAMEWORK)
        set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/bin/pkg-config")
        set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}" CACHE FILEPATH "pkg-config executable" FORCE)
        set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    else()
        _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        set(ENV{PKG_CONFIG_PATH} "")
        set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    endif()

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
        set(_gst_ios_cache_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-ios-${GStreamer_FIND_VERSION}")
        set(_gst_ios_expanded "${_gst_ios_cache_dir}/expanded")

        gstreamer_download_sdk(ios ${GStreamer_FIND_VERSION}
            "gstreamer-ios.pkg" "${_gst_ios_cache_dir}" _gst_ios_pkg)

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
    _gst_set_standard_paths(INCLUDE_PATH "${GSTREAMER_FRAMEWORK_PATH}/Headers")

    _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)

    set(ENV{PKG_CONFIG_PATH} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig")
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
    if(QT_IS_ANDROID_MULTI_ABI_EXTERNAL_PROJECT AND DEFINED QT_INTERNAL_ANDROID_MULTI_ABI_BINARY_DIR)
        set(_gst_android_build_base "${QT_INTERNAL_ANDROID_MULTI_ABI_BINARY_DIR}")
    else()
        set(_gst_android_build_base "${CMAKE_BINARY_DIR}")
    endif()
    if(QT_USE_TARGET_ANDROID_BUILD_DIR)
        set(_gst_android_build_dir "${_gst_android_build_base}/android-build-${CMAKE_PROJECT_NAME}")
    else()
        set(_gst_android_build_dir "${_gst_android_build_base}/android-build")
    endif()
    set(GStreamer_JAVA_SRC_DIR "${_gst_android_build_dir}/src")
    set(GStreamer_ASSETS_DIR "${_gst_android_build_dir}/assets")
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

function(_qgc_gstreamer_component_to_api_name INPUT_COMPONENT OUTPUT_VAR)
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" _component_snake "${INPUT_COMPONENT}")
    string(TOLOWER "${_component_snake}" _component_snake)
    set(${OUTPUT_VAR} "api_${_component_snake}" PARENT_SCOPE)
endfunction()

# Build the API target list expected by the SDK's FindGStreamer.cmake.
# The SDK iterates GSTREAMER_APIS for "gstreamer-<api>-1.0" pkg-config names.
set(GSTREAMER_APIS
    api_base
    api_gl
    api_gl_prototypes
    api_rtsp
    api_video
)
foreach(_comp IN LISTS QGCGStreamer_FIND_COMPONENTS)
    _qgc_gstreamer_component_to_api_name("${_comp}" _api_name)
    if(NOT _api_name IN_LIST GSTREAMER_APIS)
        list(APPEND GSTREAMER_APIS "${_api_name}")
    endif()
endforeach()

if(GStreamer_USE_STATIC_LIBS)
    # Use vendored GStreamer SDK cmake modules for static builds.
    # These handle cross-compilation-aware static linking (whole-archive,
    # hidden visibility), per-plugin target creation, and mobile target
    # generation (gstreamer_android-1.0.c / gst_ios_init.m).
    #
    # Let FindGStreamer populate GStreamer_EXTRA_DEPS from GSTREAMER_EXTRA_DEPS
    # via its fallback.  The extra deps ensure GStreamer::GStreamer's include
    # directories cover GL and other component headers.  FindGStreamer's
    # version extraction handles the multi-package case where
    # PC_GStreamer_VERSION is empty.

    # Propagate PKG_CONFIG_ARGN to the cache so the vendored FindGStreamer
    # module sees the --define-variable flags accumulated above.
    set(PKG_CONFIG_ARGN "${PKG_CONFIG_ARGN}" CACHE STRING "Arguments to supply to pkg-config" FORCE)

    # CPM creates a stub gstreamer-config.cmake in
    # CMAKE_FIND_PACKAGE_REDIRECTS_DIR.  In CMake 3.24+ this directory
    # takes priority over MODULE mode, so find_package(GStreamer MODULE)
    # still finds the stub instead of our vendored FindGStreamer.cmake.
    # Remove the redirect so our module is used.
    if(DEFINED CMAKE_FIND_PACKAGE_REDIRECTS_DIR)
        file(REMOVE
            "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/gstreamer-config.cmake"
            "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/gstreamer-config-version.cmake"
            "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/GStreamerConfig.cmake"
            "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/GStreamerConfigVersion.cmake"
        )
    endif()
    unset(GStreamer_FOUND CACHE)
    unset(GStreamer_FOUND)
    find_package(GStreamer REQUIRED MODULE)

    if(ANDROID OR IOS)
        set(_mobile_components ${GSTREAMER_PLUGINS} mobile)
        if(ANDROID AND EXISTS "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/fontconfig")
            list(APPEND _mobile_components fonts)
        elseif(IOS
            AND EXISTS "${GStreamer_ROOT_DIR}/share/fontconfig/fonts/Ubuntu-R.ttf"
            AND EXISTS "${GStreamer_ROOT_DIR}/etc/fonts/fonts.conf")
            list(APPEND _mobile_components fonts)
        endif()
        if(EXISTS "${GStreamer_ROOT_DIR}/etc/ssl/certs/ca-certificates.crt")
            list(APPEND _mobile_components ca_certificates)
        endif()
        find_package(GStreamerMobile REQUIRED COMPONENTS ${_mobile_components})
    endif()

    foreach(_comp IN ITEMS Core Base Video Gl GlPrototypes Rtsp)
        set(QGCGStreamer_${_comp}_FOUND TRUE)
        set(GStreamer_${_comp}_FOUND TRUE)
    endforeach()
    foreach(_comp IN LISTS QGCGStreamer_FIND_COMPONENTS)
        _qgc_gstreamer_component_to_api_name("${_comp}" _api_name)
        if(TARGET GStreamer::${_api_name})
            set(QGCGStreamer_${_comp}_FOUND TRUE)
            set(GStreamer_${_comp}_FOUND TRUE)
        endif()
    endforeach()

    # GStreamer_VERSION is already set by FindGStreamer.cmake with the
    # multi-package fallback.  Do not overwrite it with PC_GStreamer_VERSION
    # which may be empty when extra deps are passed to pkg_check_modules.

    if(GStreamer_USE_STATIC_LIBS AND NOT (ANDROID OR IOS))
        target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
    endif()

    foreach(plugin IN LISTS GSTREAMER_PLUGINS)
        if(TARGET GStreamer::${plugin})
            set(GST_PLUGIN_${plugin}_FOUND TRUE)
        else()
            set(GST_PLUGIN_${plugin}_FOUND FALSE)
        endif()
    endforeach()

else()
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

    if(NOT TARGET GStreamer::GStreamer)
        qt_add_library(GStreamer::GStreamer INTERFACE IMPORTED)

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
        endif()

        target_link_directories(GStreamer::GStreamer INTERFACE ${GSTREAMER_LIB_PATH})

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
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QGCGStreamer
    REQUIRED_VARS GStreamer_ROOT_DIR GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH
    VERSION_VAR GStreamer_VERSION
    HANDLE_COMPONENTS
)
