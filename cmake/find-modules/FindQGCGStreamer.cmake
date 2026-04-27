include(GStreamerHelpers)

if(NOT DEFINED GStreamer_FIND_VERSION)
    if(LINUX AND NOT ANDROID)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_MIN_VERSION})
    elseif(WIN32 AND NOT ANDROID)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_WIN_VERSION})
    elseif(ANDROID)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_ANDROID_VERSION})
    elseif(IOS)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_IOS_VERSION})
    elseif(MACOS)
        set(GStreamer_FIND_VERSION ${QGC_CONFIG_GSTREAMER_MACOS_VERSION})
    else()
        message(WARNING "GStreamer: unrecognized platform — using fallback version ${QGC_CONFIG_GSTREAMER_VERSION}")
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

set(PKG_CONFIG_ARGN "" CACHE STRING "Extra arguments for pkg-config" FORCE)
set(GStreamer_AUTO_DOWNLOADED FALSE)

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

function(_qgc_find_win_sdk_root EXTRACTED_DIR OUT_VAR)
    if(EXISTS "${EXTRACTED_DIR}/bin/pkg-config.exe")
        set(${OUT_VAR} "${EXTRACTED_DIR}" PARENT_SCOPE)
        return()
    endif()
    file(GLOB_RECURSE _pkg_config_files "${EXTRACTED_DIR}/pkg-config.exe")
    if(_pkg_config_files)
        list(GET _pkg_config_files 0 _first_pkg_config)
        cmake_path(GET _first_pkg_config PARENT_PATH _bin_dir)
        cmake_path(GET _bin_dir PARENT_PATH _found_root)
        set(${OUT_VAR} "${_found_root}" PARENT_SCOPE)
    else()
        set(${OUT_VAR} "" PARENT_SCOPE)
    endif()
endfunction()

function(_qgc_validate_expanded_pkg EXPANDED_DIR LABEL)
    file(GLOB _payloads "${EXPANDED_DIR}/*.pkg/Payload")
    if(NOT _payloads)
        file(REMOVE_RECURSE "${EXPANDED_DIR}")
        message(FATAL_ERROR
            "pkgutil expanded GStreamer ${LABEL} package but no payloads were found in ${EXPANDED_DIR}")
    endif()
endfunction()

macro(_qgc_discover_windows_sdk)
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

        if(CPM_SOURCE_CACHE)
            set(_gst_win_cache_dir "${CPM_SOURCE_CACHE}/gstreamer-win-${_gst_win_arch}-${GStreamer_FIND_VERSION}")
        else()
            set(_gst_win_cache_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-win-${_gst_win_arch}-${GStreamer_FIND_VERSION}")
        endif()
        set(_gst_win_extracted "${_gst_win_cache_dir}/sdk")

        gstreamer_download_sdk(${_gst_win_platform} ${GStreamer_FIND_VERSION}
            "gstreamer-${_gst_win_arch}.exe" "${_gst_win_cache_dir}" _gst_win_exe)

        set(_gst_win_root "")
        if(EXISTS "${_gst_win_extracted}")
            _qgc_find_win_sdk_root("${_gst_win_extracted}" _gst_win_root)
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
                ERROR_VARIABLE _installer_err
                TIMEOUT 600
            )
            if(NOT _installer_rc EQUAL 0)
                file(REMOVE_RECURSE "${_gst_win_extracted}")
                message(FATAL_ERROR "GStreamer installer failed (exit code: ${_installer_rc}).\n"
                    "stderr: ${_installer_err}\n"
                    "See installer log: ${_gst_win_installer_log}")
            endif()

            _qgc_find_win_sdk_root("${_gst_win_extracted}" _gst_win_root)
            if(_gst_win_root AND NOT _gst_win_root STREQUAL "${_gst_win_extracted}")
                message(STATUS "GStreamer: SDK root at ${_gst_win_root}")
            endif()

            if(NOT _gst_win_root
               OR NOT EXISTS "${_gst_win_root}/bin/pkg-config.exe"
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

    _gst_configure_pkg_config(
        PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/bin/pkg-config.exe"
        LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        DONT_DEFINE_PREFIX
    )
endmacro()

macro(_qgc_discover_linux_sdk)
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

    # Prepend (not replace) so system glib/gobject .pc files remain discoverable
    set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
endmacro()

macro(_qgc_discover_android_sdk)
    if(NOT DEFINED GStreamer_ROOT_DIR)
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

    if(CMAKE_HOST_WIN32)
        _gst_configure_pkg_config(
            PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/share/gst-android/ndk-build/tools/windows/pkg-config.exe"
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
            DONT_DEFINE_PREFIX
        )
    elseif(CMAKE_HOST_UNIX)
        if(CMAKE_HOST_APPLE)
            _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        endif()
        _gst_configure_pkg_config(
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
endmacro()

macro(_qgc_discover_macos_sdk)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        if(EXISTS "/Library/Frameworks/GStreamer.framework")
            set(GStreamer_ROOT_DIR "/Library/Frameworks/GStreamer.framework/Versions/1.0")
        else()
            foreach(_brew_prefix IN ITEMS "/opt/homebrew/opt/gstreamer" "/usr/local/opt/gstreamer")
                if(EXISTS "${_brew_prefix}")
                    set(GStreamer_ROOT_DIR "${_brew_prefix}")
                    set(GStreamer_USE_FRAMEWORK OFF)
                    message(STATUS "GStreamer: Using Homebrew at ${GStreamer_ROOT_DIR}")
                    break()
                endif()
            endforeach()
        endif()
    endif()

    if(NOT DEFINED GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}")
        if(CPM_SOURCE_CACHE)
            set(_gst_mac_cache_dir "${CPM_SOURCE_CACHE}/gstreamer-mac-${GStreamer_FIND_VERSION}")
        else()
            set(_gst_mac_cache_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-mac-${GStreamer_FIND_VERSION}")
        endif()
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
        _gst_configure_pkg_config(
            PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/bin/pkg-config"
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    else()
        _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        _gst_configure_pkg_config(
            LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        )
    endif()
endmacro()

macro(_qgc_discover_ios_sdk)
    if(NOT CMAKE_HOST_APPLE)
        message(FATAL_ERROR "GStreamer for iOS can only be built on macOS")
    endif()

    # ── User-supplied root ────────────────────────────────────────────────────
    if(NOT DEFINED GStreamer_ROOT_DIR)
        # Check well-known system install locations (framework and xcframework).
        if(EXISTS "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.xcframework")
            set(_gst_ios_system_xcfw "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.xcframework")
        elseif(EXISTS "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
            set(GSTREAMER_FRAMEWORK_PATH "/Library/Developer/GStreamer/iPhone.sdk/GStreamer.framework")
            set(GStreamer_ROOT_DIR "${GSTREAMER_FRAMEWORK_PATH}/Versions/1.0")
        endif()
    endif()

    # ── Auto-download / expand ────────────────────────────────────────────────
    if((NOT DEFINED GStreamer_ROOT_DIR OR NOT EXISTS "${GStreamer_ROOT_DIR}") AND NOT DEFINED _gst_ios_system_xcfw)
        if(CPM_SOURCE_CACHE)
            set(_gst_ios_cache_dir "${CPM_SOURCE_CACHE}/gstreamer-ios-${GStreamer_FIND_VERSION}")
        else()
            set(_gst_ios_cache_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-ios-${GStreamer_FIND_VERSION}")
        endif()
        set(_gst_ios_expanded "${_gst_ios_cache_dir}/expanded")

        gstreamer_download_sdk(ios ${GStreamer_FIND_VERSION}
            "gstreamer-ios.pkg" "${_gst_ios_cache_dir}" _gst_ios_pkg)

        # Anchors that prove the expansion is complete for either SDK layout.
        set(_gst_ios_anchor_globs
            "${_gst_ios_expanded}/*/GStreamer.xcframework/Info.plist"
            "${_gst_ios_expanded}/*/GStreamer.framework/Headers/gst/gst.h"
            "${_gst_ios_expanded}/*/GStreamer.framework/Versions/1.0/Headers/gst/gst.h"
            "${_gst_ios_expanded}/*/GStreamer.framework/GStreamer"
            "${_gst_ios_expanded}/*/GStreamer.framework/Versions/1.0/GStreamer"
            "${_gst_ios_expanded}/*/GStreamer.framework/Info.plist"
        )

        if(EXISTS "${_gst_ios_expanded}")
            set(_cached_anchor "")
            foreach(_glob IN LISTS _gst_ios_anchor_globs)
                file(GLOB_RECURSE _cached_anchor "${_glob}")
                if(_cached_anchor)
                    break()
                endif()
            endforeach()
            if(NOT _cached_anchor)
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

        # ── xcframework detection (GStreamer 1.28+) ───────────────────────────
        # Check xcframework first; fall through to .framework for older SDKs.
        file(GLOB_RECURSE _xcfw_info_plists LIST_DIRECTORIES false
            "${_gst_ios_expanded}/*/GStreamer.xcframework/Info.plist"
            "${_gst_ios_expanded}/GStreamer.xcframework/Info.plist"
        )
        if(NOT _xcfw_info_plists)
            # Broader walk in case the pkg nests the xcframework further.
            file(GLOB_RECURSE _all_dirs LIST_DIRECTORIES true "${_gst_ios_expanded}/*")
            foreach(_d IN LISTS _all_dirs)
                if(IS_DIRECTORY "${_d}" AND _d MATCHES "/GStreamer\.xcframework$")
                    if(EXISTS "${_d}/Info.plist")
                        list(APPEND _xcfw_info_plists "${_d}/Info.plist")
                        break()
                    endif()
                endif()
            endforeach()
        endif()

        if(_xcfw_info_plists)
            list(GET _xcfw_info_plists 0 _xcfw_info_first)
            cmake_path(GET _xcfw_info_first PARENT_PATH _gst_ios_system_xcfw)
        else()
            # ── .framework fallback (pre-1.28 SDK) ───────────────────────────
            # Older SDK versions ship GStreamer.framework; try anchors first.
            set(_anchor_hit "")
            foreach(_glob IN LISTS _gst_ios_anchor_globs)
                file(GLOB_RECURSE _anchor_hit "${_glob}")
                if(_anchor_hit)
                    break()
                endif()
            endforeach()

            if(_anchor_hit)
                list(GET _anchor_hit 0 _anchor_first)
                set(_walk "${_anchor_first}")
                while(_walk AND NOT _walk MATCHES "GStreamer\.framework$")
                    cmake_path(GET _walk PARENT_PATH _walk)
                    if(_walk STREQUAL "/" OR _walk STREQUAL "")
                        break()
                    endif()
                endwhile()
                if(_walk MATCHES "GStreamer\.framework$")
                    set(GSTREAMER_FRAMEWORK_PATH "${_walk}")
                endif()
            endif()

            if(NOT GSTREAMER_FRAMEWORK_PATH)
                file(GLOB_RECURSE _all_dirs LIST_DIRECTORIES true "${_gst_ios_expanded}/*")
                foreach(_d IN LISTS _all_dirs)
                    if(IS_DIRECTORY "${_d}" AND _d MATCHES "/GStreamer\.framework$")
                        if(EXISTS "${_d}/Headers" OR EXISTS "${_d}/Versions/1.0/Headers")
                            set(GSTREAMER_FRAMEWORK_PATH "${_d}")
                            break()
                        elseif(NOT GSTREAMER_FRAMEWORK_PATH)
                            set(GSTREAMER_FRAMEWORK_PATH "${_d}")
                        endif()
                    endif()
                endforeach()
            endif()

            if(NOT GSTREAMER_FRAMEWORK_PATH)
                file(GLOB _top_entries LIST_DIRECTORIES true "${_gst_ios_expanded}/*")
                file(GLOB_RECURSE _all_frameworks LIST_DIRECTORIES true
                    "${_gst_ios_expanded}/*.framework")
                file(GLOB_RECURSE _all_xcframeworks LIST_DIRECTORIES true
                    "${_gst_ios_expanded}/*.xcframework")
                file(GLOB_RECURSE _all_in_expanded "${_gst_ios_expanded}/*")
                list(LENGTH _all_in_expanded _n)
                string(REPLACE ";" "\n  " _top_entries_str "${_top_entries}")
                string(REPLACE ";" "\n  " _all_frameworks_str "${_all_frameworks}")
                string(REPLACE ";" "\n  " _all_xcframeworks_str "${_all_xcframeworks}")
                message(FATAL_ERROR
                    "Could not locate GStreamer.xcframework or GStreamer.framework in expanded iOS SDK at"
                    " '${_gst_ios_expanded}' (${_n} entries). The .pkg layout may"
                    " have changed; check the pkgutil --expand-full output.\n"
                    "Top-level entries:\n  ${_top_entries_str}\n"
                    "All *.framework directories:\n  ${_all_frameworks_str}\n"
                    "All *.xcframework directories:\n  ${_all_xcframeworks_str}")
            endif()

            set(GStreamer_ROOT_DIR "${GSTREAMER_FRAMEWORK_PATH}/Versions/1.0")
        endif()
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    # ── xcframework path: pick the right slice ────────────────────────────────
    if(DEFINED _gst_ios_system_xcfw)
        set(GStreamer_USE_XCFRAMEWORK ON)

        # Select slice based on sysroot: iphoneos=device, iphonesimulator=simulator.
        if(CMAKE_OSX_SYSROOT MATCHES "iphonesimulator")
            set(_xcfw_slice "ios-arm64_x86_64-simulator")
        else()
            set(_xcfw_slice "ios-arm64")
        endif()

        set(_xcfw_slice_dir "${_gst_ios_system_xcfw}/${_xcfw_slice}")
        if(NOT EXISTS "${_xcfw_slice_dir}")
            message(FATAL_ERROR
                "GStreamer xcframework slice '${_xcfw_slice}' not found in ${_gst_ios_system_xcfw}.
"
                "Available slices: check ${_gst_ios_system_xcfw}/Info.plist AvailableLibraries.")
        endif()

        # Synthetic path vars so guards and consumers have consistent variables.
        # xcframework has no lib/ or lib/gstreamer-1.0/ — all code is in libGStreamer.a.
        set(GStreamer_ROOT_DIR       "${_xcfw_slice_dir}")
        set(GSTREAMER_XCFRAMEWORK_PATH "${_gst_ios_system_xcfw}")
        set(GSTREAMER_XCFRAMEWORK_LIB  "${_xcfw_slice_dir}/libGStreamer.a")
        set(GSTREAMER_INCLUDE_PATH  "${_xcfw_slice_dir}/Headers")
        # GSTREAMER_LIB_PATH and GSTREAMER_PLUGIN_PATH are synthetic — they point
        # to the slice dir itself because there is no lib/ subdirectory in an xcframework.
        set(GSTREAMER_LIB_PATH    "${_xcfw_slice_dir}")
        set(GSTREAMER_PLUGIN_PATH "${_xcfw_slice_dir}")

        _gst_normalize_and_validate_root()
        return()
    endif()

    # ── Classic .framework path ───────────────────────────────────────────────
    _gst_normalize_and_validate_root()

    set(GStreamer_USE_FRAMEWORK ON)
    if(NOT DEFINED GSTREAMER_FRAMEWORK_PATH)
        set(GSTREAMER_FRAMEWORK_PATH "${GStreamer_ROOT_DIR}/../..")
        cmake_path(NORMAL_PATH GSTREAMER_FRAMEWORK_PATH)
    endif()
    _gst_set_standard_paths(INCLUDE_PATH "${GSTREAMER_FRAMEWORK_PATH}/Headers")

    # Cerbero iOS framework lays out libraries under Versions/1.0/lib but the
    # framework root also exposes Libraries -> Versions/Current/lib symlinks;
    # if the default ${GStreamer_ROOT_DIR}/lib doesn't exist, try alternatives.
    if(NOT EXISTS "${GSTREAMER_LIB_PATH}")
        foreach(_cand IN ITEMS
            "${GStreamer_ROOT_DIR}/Libraries"
            "${GSTREAMER_FRAMEWORK_PATH}/Libraries"
            "${GSTREAMER_FRAMEWORK_PATH}/lib"
        )
            if(EXISTS "${_cand}")
                set(GSTREAMER_LIB_PATH "${_cand}")
                set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
                break()
            endif()
        endforeach()
    endif()

    _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
    _gst_configure_pkg_config(
        LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
    )
endmacro()

# Dispatch to the appropriate platform discovery macro
if(WIN32 AND NOT ANDROID)
    _qgc_discover_windows_sdk()
elseif(LINUX AND NOT ANDROID)
    _qgc_discover_linux_sdk()
elseif(ANDROID)
    _qgc_discover_android_sdk()
elseif(MACOS AND NOT IOS)
    _qgc_discover_macos_sdk()
elseif(IOS)
    _qgc_discover_ios_sdk()
endif()

# xcframework sets GSTREAMER_LIB_PATH / GSTREAMER_PLUGIN_PATH to the slice dir
# (which has no lib/ subdir), so the existence checks still pass.
if(NOT GStreamer_USE_XCFRAMEWORK)
    set(_gst_required_paths
        "GStreamer_ROOT_DIR=${GStreamer_ROOT_DIR}"
        "GSTREAMER_LIB_PATH=${GSTREAMER_LIB_PATH}"
        "GSTREAMER_PLUGIN_PATH=${GSTREAMER_PLUGIN_PATH}"
        "GSTREAMER_INCLUDE_PATH=${GSTREAMER_INCLUDE_PATH}"
    )
    set(_gst_missing_paths)
    if(NOT EXISTS "${GStreamer_ROOT_DIR}")
        list(APPEND _gst_missing_paths "GStreamer_ROOT_DIR=${GStreamer_ROOT_DIR}")
    endif()
    if(NOT EXISTS "${GSTREAMER_LIB_PATH}")
        list(APPEND _gst_missing_paths "GSTREAMER_LIB_PATH=${GSTREAMER_LIB_PATH}")
    endif()
    if(NOT EXISTS "${GSTREAMER_PLUGIN_PATH}")
        list(APPEND _gst_missing_paths "GSTREAMER_PLUGIN_PATH=${GSTREAMER_PLUGIN_PATH}")
    endif()
    if(NOT EXISTS "${GSTREAMER_INCLUDE_PATH}")
        list(APPEND _gst_missing_paths "GSTREAMER_INCLUDE_PATH=${GSTREAMER_INCLUDE_PATH}")
    endif()
    if(_gst_missing_paths)
        string(REPLACE ";" "\n  " _gst_missing_str "${_gst_missing_paths}")
        message(FATAL_ERROR
            "GStreamer: required directories do not exist on disk:\n  ${_gst_missing_str}\n"
            "GSTREAMER_FRAMEWORK_PATH=${GSTREAMER_FRAMEWORK_PATH}\n"
            "Check installation or set GStreamer_ROOT_DIR.")
    endif()

    if(GStreamer_USE_FRAMEWORK AND NOT EXISTS "${GSTREAMER_FRAMEWORK_PATH}")
        message(FATAL_ERROR "GStreamer: Could not locate framework at ${GSTREAMER_FRAMEWORK_PATH}")
    endif()
else()
    if(NOT EXISTS "${GSTREAMER_XCFRAMEWORK_LIB}")
        message(FATAL_ERROR "GStreamer: xcframework library not found at ${GSTREAMER_XCFRAMEWORK_LIB}")
    endif()
endif()

function(_qgc_gstreamer_component_to_api_name INPUT_COMPONENT OUTPUT_VAR)
    string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" _component_snake "${INPUT_COMPONENT}")
    string(TOLOWER "${_component_snake}" _component_snake)
    set(${OUTPUT_VAR} "api_${_component_snake}" PARENT_SCOPE)
endfunction()

set(GSTREAMER_APIS
    api_base
    api_gl
    api_gl_prototypes
    api_rtsp
    api_video
)
# Map QGCGStreamer components to api_ names, skipping Core (already covered
# by the main gstreamer-1.0 pkg-config query — there is no gstreamer-core-1.0.pc).
foreach(_comp IN LISTS QGCGStreamer_FIND_COMPONENTS)
    if(_comp STREQUAL "Core")
        continue()
    endif()
    _qgc_gstreamer_component_to_api_name("${_comp}" _api_name)
    if(NOT _api_name IN_LIST GSTREAMER_APIS)
        list(APPEND GSTREAMER_APIS "${_api_name}")
    endif()
endforeach()

if(NOT DEFINED GSTREAMER_EXTRA_DEPS)
    # Derive from GSTREAMER_APIS to avoid maintaining a duplicate list.
    # Each api_foo entry maps to gstreamer-foo-1.0 (with underscores → hyphens).
    set(GSTREAMER_EXTRA_DEPS)
    foreach(_api IN LISTS GSTREAMER_APIS)
        string(REGEX REPLACE "^api_(.+)" "\\1" _pc_name "${_api}")
        string(REPLACE "_" "-" _pc_name "${_pc_name}")
        list(APPEND GSTREAMER_EXTRA_DEPS "gstreamer-${_pc_name}-1.0")
    endforeach()
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
        videoparsersbad
        vpx
    )
    # Deferred on Linux — actual version may differ from the minimum (see finalization below)
    if(NOT LINUX)
        if(GStreamer_FIND_VERSION VERSION_GREATER_EQUAL "1.22")
            list(APPEND GSTREAMER_PLUGINS videoconvertscale)
        else()
            list(APPEND GSTREAMER_PLUGINS videoconvert videoscale)
        endif()
    endif()
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
    if(NOT GStreamer_USE_XCFRAMEWORK)
        # xcframework bundles gio modules into libGStreamer.a; no separate module dir.
        set(G_IO_MODULES openssl)
        set(G_IO_MODULES_PATH "${GStreamer_ROOT_DIR}/lib/gio/modules")
    endif()
    set(GStreamer_ASSETS_DIR "${CMAKE_BINARY_DIR}/assets")
endif()

set(GStreamer_ROOT_DIR "${GStreamer_ROOT_DIR}" CACHE PATH "GStreamer SDK root directory" FORCE)

if(GStreamer_USE_FRAMEWORK)
    list(APPEND CMAKE_FRAMEWORK_PATH "${GSTREAMER_FRAMEWORK_PATH}")
endif()

if(GStreamer_USE_XCFRAMEWORK)
    # ── xcframework path: create IMPORTED targets directly from the fat .a ───
    # No pkg-config or .framework; all APIs and plugins live in one archive.
    if(NOT TARGET GStreamer::GStreamer)
        add_library(GStreamer_static STATIC IMPORTED GLOBAL)
        set_target_properties(GStreamer_static PROPERTIES
            IMPORTED_LOCATION "${GSTREAMER_XCFRAMEWORK_LIB}"
        )
        add_library(GStreamer::GStreamer INTERFACE IMPORTED GLOBAL)
        target_link_libraries(GStreamer::GStreamer INTERFACE
            GStreamer_static
        )
        target_include_directories(GStreamer::GStreamer INTERFACE
            "${GSTREAMER_INCLUDE_PATH}"
            "${GSTREAMER_INCLUDE_PATH}/gstreamer-1.0"
            "${GSTREAMER_INCLUDE_PATH}/glib-2.0"
        )
        # System frameworks required by GStreamer on iOS.
        find_library(_xcfw_foundation     Foundation     REQUIRED)
        find_library(_xcfw_avfoundation   AVFoundation   REQUIRED)
        find_library(_xcfw_audiotoolbox   AudioToolbox   REQUIRED)
        find_library(_xcfw_videotoolbox   VideoToolbox   REQUIRED)
        find_library(_xcfw_coremedia      CoreMedia      REQUIRED)
        find_library(_xcfw_corevideo      CoreVideo      REQUIRED)
        find_library(_xcfw_coreaudio      CoreAudio      REQUIRED)
        find_library(_xcfw_coregraphics   CoreGraphics   REQUIRED)
        find_library(_xcfw_security       Security       REQUIRED)
        find_library(_xcfw_opengles       OpenGLES       REQUIRED)
        find_library(_xcfw_uikit          UIKit          REQUIRED)
        find_library(_xcfw_corefoundation CoreFoundation REQUIRED)
        target_link_libraries(GStreamer::GStreamer INTERFACE
            "${_xcfw_foundation}"
            "${_xcfw_avfoundation}"
            "${_xcfw_audiotoolbox}"
            "${_xcfw_videotoolbox}"
            "${_xcfw_coremedia}"
            "${_xcfw_corevideo}"
            "${_xcfw_coreaudio}"
            "${_xcfw_coregraphics}"
            "${_xcfw_security}"
            "${_xcfw_opengles}"
            "${_xcfw_uikit}"
            "${_xcfw_corefoundation}"
            "-lresolv" "-liconv" "-lz" "-lbz2"
        )
        target_compile_definitions(GStreamer::GStreamer INTERFACE
            QGC_GST_STATIC_BUILD
        )
        unset(_xcfw_foundation CACHE)
        unset(_xcfw_avfoundation CACHE)
        unset(_xcfw_audiotoolbox CACHE)
        unset(_xcfw_videotoolbox CACHE)
        unset(_xcfw_coremedia CACHE)
        unset(_xcfw_corevideo CACHE)
        unset(_xcfw_coreaudio CACHE)
        unset(_xcfw_coregraphics CACHE)
        unset(_xcfw_security CACHE)
        unset(_xcfw_opengles CACHE)
        unset(_xcfw_uikit CACHE)
        unset(_xcfw_corefoundation CACHE)
    endif()

    # All API component targets alias the single mega-library.
    foreach(_xcfw_comp IN ITEMS api_base api_gl api_gl_prototypes api_rtsp api_video api_app)
        if(NOT TARGET GStreamer::${_xcfw_comp})
            add_library(GStreamer::${_xcfw_comp} INTERFACE IMPORTED GLOBAL)
            target_link_libraries(GStreamer::${_xcfw_comp} INTERFACE GStreamer::GStreamer)
        endif()
    endforeach()

    # Build the xcframework mobile init shim — calls gst_init_static_plugins().
    if(NOT TARGET GStreamer::mobile)
        enable_language(OBJC OBJCXX)

        # GStreamer 1.28+ ships every plugin compiled into libGStreamer.a but
        # provides no auto-registration entrypoint; enumerate plugin descriptors
        # from the archive and emit explicit GST_PLUGIN_STATIC_REGISTER() calls.
        find_program(_xcfw_nm NAMES nm llvm-nm REQUIRED)
        execute_process(
            COMMAND "${_xcfw_nm}" -gjU "${GSTREAMER_XCFRAMEWORK_LIB}"
            OUTPUT_VARIABLE _xcfw_nm_out
            ERROR_QUIET
            RESULT_VARIABLE _xcfw_nm_rc
        )
        if(NOT _xcfw_nm_rc EQUAL 0)
            message(FATAL_ERROR "nm failed on ${GSTREAMER_XCFRAMEWORK_LIB} (rc=${_xcfw_nm_rc})")
        endif()
        string(REGEX MATCHALL "_gst_plugin_[A-Za-z0-9_]+_get_desc" _xcfw_descs "${_xcfw_nm_out}")
        list(REMOVE_DUPLICATES _xcfw_descs)
        set(_xcfw_decl "")
        set(_xcfw_reg  "")
        foreach(_sym IN LISTS _xcfw_descs)
            string(REGEX REPLACE "^_gst_plugin_(.+)_get_desc$" "\\1" _name "${_sym}")
            string(APPEND _xcfw_decl "GST_PLUGIN_STATIC_DECLARE(${_name});\n")
            string(APPEND _xcfw_reg  "    GST_PLUGIN_STATIC_REGISTER(${_name});\n")
        endforeach()
        list(LENGTH _xcfw_descs _xcfw_n)
        message(STATUS "GStreamer xcframework: registering ${_xcfw_n} static plugins")
        set(GST_STATIC_PLUGIN_DECLARES "${_xcfw_decl}")
        set(GST_STATIC_PLUGIN_REGISTERS "${_xcfw_reg}")

        set(_xcfw_shim "${CMAKE_BINARY_DIR}/${GStreamer_Mobile_MODULE_NAME}.m")
        configure_file(
            "${CMAKE_CURRENT_LIST_DIR}/GStreamer/gst_ios_xcframework_init.m.in"
            "${_xcfw_shim}"
            @ONLY
        )
        add_library(GStreamerMobileXcfw SHARED)
        target_sources(GStreamerMobileXcfw PRIVATE "${_xcfw_shim}")
        set_source_files_properties("${_xcfw_shim}" PROPERTIES LANGUAGE OBJC GENERATED TRUE)
        target_link_libraries(GStreamerMobileXcfw PRIVATE GStreamer::GStreamer)
        set_target_properties(GStreamerMobileXcfw PROPERTIES
            LIBRARY_OUTPUT_NAME ${GStreamer_Mobile_MODULE_NAME}
            LINKER_LANGUAGE OBJCXX
            FRAMEWORK TRUE
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER org.gstreamer.GStreamerMobile
        )
        add_library(GStreamer::mobile ALIAS GStreamerMobileXcfw)
        add_library(GStreamerMobile ALIAS GStreamerMobileXcfw)
        set(GStreamerMobile_FOUND TRUE)
        set(GStreamerMobile_mobile_FOUND TRUE)
    endif()

    set(GStreamer_FOUND TRUE)
    set(GStreamer_VERSION "${GStreamer_FIND_VERSION}")
else()

if(GStreamer_USE_STATIC_LIBS)
    list(APPEND PKG_CONFIG_ARGN "--static")
endif()

find_package(PkgConfig REQUIRED QUIET)

list(PREPEND CMAKE_PREFIX_PATH ${GStreamer_ROOT_DIR})

set(PKG_CONFIG_ARGN "${PKG_CONFIG_ARGN}" CACHE STRING "Arguments to supply to pkg-config" FORCE)

# CPM creates a stub gstreamer-config.cmake that shadows our vendored module
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
find_package(GStreamer ${QGC_CONFIG_GSTREAMER_MIN_VERSION} REQUIRED MODULE)

# Apple framework overlay: when using framework layout, link the framework
# directly and add its Headers to the include path.
if(APPLE AND GStreamer_USE_FRAMEWORK AND TARGET GStreamer::GStreamer)
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

endif() # GStreamer_USE_XCFRAMEWORK

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

if(GStreamer_USE_STATIC_LIBS)
    target_compile_definitions(GStreamer::GStreamer INTERFACE QGC_GST_STATIC_BUILD)
    if(ANDROID)
        # FFmpeg static libs have arch-specific assembly using page-relative
        # relocations against global symbols. -Bsymbolic guarantees no symbol
        # interposition, making those relocations valid in the final .so.
        target_link_options(GStreamer::GStreamer INTERFACE "-Wl,-Bsymbolic")
    endif()
endif()

foreach(plugin IN LISTS GSTREAMER_PLUGINS)
    if(TARGET GStreamer::${plugin})
        set(GST_PLUGIN_${plugin}_FOUND TRUE)
    else()
        set(GST_PLUGIN_${plugin}_FOUND FALSE)
    endif()
endforeach()

if(LINUX AND NOT "videoconvertscale" IN_LIST GSTREAMER_PLUGINS
        AND NOT "videoconvert" IN_LIST GSTREAMER_PLUGINS)
    if(GStreamer_VERSION VERSION_GREATER_EQUAL "1.22")
        list(APPEND GSTREAMER_PLUGINS videoconvertscale)
    else()
        list(APPEND GSTREAMER_PLUGINS videoconvert videoscale)
    endif()
endif()

if(NOT GStreamer_USE_STATIC_LIBS AND NOT GStreamer_USE_XCFRAMEWORK AND EXISTS "${GSTREAMER_PLUGIN_PATH}")
    set(_gst_missing_plugins)
    if(WIN32)
        set(_gst_plugin_glob "gst*.dll")
    elseif(APPLE)
        set(_gst_plugin_glob "libgst*.dylib")
    else()
        set(_gst_plugin_glob "libgst*.so")
    endif()
    file(GLOB _gst_available_plugins "${GSTREAMER_PLUGIN_PATH}/${_gst_plugin_glob}")
    set(_gst_available_basenames)
    foreach(_path IN LISTS _gst_available_plugins)
        get_filename_component(_fname "${_path}" NAME)
        if(WIN32)
            string(REGEX MATCH "^gst([a-zA-Z0-9]+)" _m "${_fname}")
        else()
            string(REGEX MATCH "^libgst([a-zA-Z0-9]+)" _m "${_fname}")
        endif()
        if(_m)
            list(APPEND _gst_available_basenames "${CMAKE_MATCH_1}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _gst_available_basenames)
    foreach(_plugin IN LISTS GSTREAMER_PLUGINS)
        if(NOT _plugin IN_LIST _gst_available_basenames)
            list(APPEND _gst_missing_plugins "${_plugin}")
        endif()
    endforeach()
    if(_gst_missing_plugins)
        message(WARNING "GStreamer: The following plugins are listed in GSTREAMER_PLUGINS "
            "but not found in ${GSTREAMER_PLUGIN_PATH}: ${_gst_missing_plugins}\n"
            "Video features depending on these plugins will not work at runtime.")
    endif()
endif()

if(GStreamer_VERSION AND TARGET GStreamer::GStreamer)
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _gst_ver_match "${GStreamer_VERSION}")
    if(_gst_ver_match)
        target_compile_definitions(GStreamer::GStreamer INTERFACE
            QGC_GST_BUILD_VERSION_MAJOR=${CMAKE_MATCH_1}
            QGC_GST_BUILD_VERSION_MINOR=${CMAKE_MATCH_2}
        )
    endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QGCGStreamer
    REQUIRED_VARS GStreamer_ROOT_DIR GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH
    VERSION_VAR GStreamer_VERSION
    HANDLE_COMPONENTS
)
