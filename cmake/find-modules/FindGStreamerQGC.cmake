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

# PKG_CONFIG_ARGN is consumed by CMake's FindPkgConfig module (3.22+).
# Extra flags appended here are forwarded to every pkg_check_modules() call.
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

if(WIN32)
    # Determine target architecture for SDK selection.
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

        # GStreamer 1.28+ ships a single .exe installer (combined runtime + devel).
        gstreamer_get_package_url(${_gst_win_platform} ${GStreamer_FIND_VERSION} _gst_win_url)
        gstreamer_get_s3_mirror_url(${_gst_win_platform} ${GStreamer_FIND_VERSION} _gst_win_s3_url)
        gstreamer_fetch_checksum(${_gst_win_platform} ${GStreamer_FIND_VERSION} _gst_win_hash)

        set(_gst_win_cache_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-win-${GStreamer_FIND_VERSION}")
        set(_gst_win_extracted "${_gst_win_cache_dir}/sdk")

        gstreamer_resilient_download(
            URLS "${_gst_win_url}" "${_gst_win_s3_url}"
            FILENAME "gstreamer-${_gst_win_arch}.exe"
            DESTINATION_DIR "${_gst_win_cache_dir}"
            RESULT_VAR _gst_win_exe
            EXPECTED_HASH "${_gst_win_hash}"
        )

        # Locate SDK root — installers may nest files under 1.0/msvc_<arch>/.
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

        # Validate completeness; install if incomplete.
        if(NOT _gst_win_root
           OR NOT EXISTS "${_gst_win_root}/lib/gstreamer-1.0"
           OR NOT EXISTS "${_gst_win_root}/lib/pkgconfig/gstreamer-1.0.pc")
            file(REMOVE_RECURSE "${_gst_win_extracted}")
            cmake_path(NATIVE_PATH _gst_win_exe _gst_win_exe_native)
            cmake_path(NATIVE_PATH _gst_win_extracted _gst_win_extracted_native)
            set(_gst_win_installer_log "${_gst_win_cache_dir}/installer-${_gst_win_arch}.log")
            cmake_path(NATIVE_PATH _gst_win_installer_log _gst_win_installer_log_native)

            # GStreamer 1.24+ uses Inno Setup for Windows .exe installers.
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

            # Re-locate SDK root after extraction.
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

    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")
    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")

    set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/bin/pkg-config.exe")
    # CACHE FORCE ensures find_package(PkgConfig) sees the bundled exe even when
    # no system pkg-config exists (which leaves the cache as NOTFOUND).
    set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}" CACHE FILEPATH "pkg-config executable" FORCE)
    # Isolate pkg-config resolution to the selected SDK to avoid host .pc leakage.
    set(ENV{PKG_CONFIG_PATH} "")
    set(ENV{PKG_CONFIG_LIBDIR} "${GSTREAMER_LIB_PATH}/pkgconfig;${GSTREAMER_PLUGIN_PATH}/pkgconfig")
    list(APPEND PKG_CONFIG_ARGN
        --dont-define-prefix
        --define-variable=prefix=${GStreamer_ROOT_DIR}
        --define-variable=libdir=${GSTREAMER_LIB_PATH}
        --define-variable=includedir=${GSTREAMER_INCLUDE_PATH}
    )

elseif(LINUX)
    if(NOT DEFINED GStreamer_ROOT_DIR)
        set(GStreamer_ROOT_DIR "/usr")
    endif()

    _gst_normalize_and_validate_root()

    # Try multiarch path first (Debian/Ubuntu), then lib64 (Fedora/RHEL), then lib (Arch)
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

    # Isolate cross-compilation: clear host paths, restrict to Android SDK.
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

elseif(MACOS)
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
        gstreamer_get_package_url(macos ${GStreamer_FIND_VERSION} _gst_mac_url)
        gstreamer_get_s3_mirror_url(macos ${GStreamer_FIND_VERSION} _gst_mac_s3_url)
        gstreamer_fetch_checksum(macos ${GStreamer_FIND_VERSION} _gst_mac_hash)
        gstreamer_get_package_url(macos_devel ${GStreamer_FIND_VERSION} _gst_mac_devel_url)
        gstreamer_get_s3_mirror_url(macos_devel ${GStreamer_FIND_VERSION} _gst_mac_devel_s3_url)
        gstreamer_fetch_checksum(macos_devel ${GStreamer_FIND_VERSION} _gst_mac_devel_hash)

        # Version-scoped cache avoids stale SDK reuse across branch/version changes.
        set(_gst_mac_cache_dir "${CMAKE_BINARY_DIR}/_deps/gstreamer-mac-${GStreamer_FIND_VERSION}")
        set(_gst_mac_expanded "${_gst_mac_cache_dir}/expanded")
        set(_gst_mac_devel_expanded "${_gst_mac_cache_dir}/expanded-devel")
        set(_gst_mac_root "${_gst_mac_cache_dir}/root")
        set(_gst_mac_required_plugin_dir "${_gst_mac_root}/lib/gstreamer-1.0")
        set(_gst_mac_required_include_dir "${_gst_mac_root}/include/gstreamer-1.0")
        set(_gst_mac_required_pc_file "${_gst_mac_root}/lib/pkgconfig/gstreamer-1.0.pc")

        gstreamer_resilient_download(
            URLS "${_gst_mac_url}" "${_gst_mac_s3_url}"
            FILENAME "gstreamer.pkg"
            DESTINATION_DIR "${_gst_mac_cache_dir}"
            RESULT_VAR _gst_mac_pkg
            EXPECTED_HASH "${_gst_mac_hash}"
        )
        gstreamer_resilient_download(
            URLS "${_gst_mac_devel_url}" "${_gst_mac_devel_s3_url}"
            FILENAME "gstreamer-devel.pkg"
            DESTINATION_DIR "${_gst_mac_cache_dir}"
            RESULT_VAR _gst_mac_devel_pkg
            EXPECTED_HASH "${_gst_mac_devel_hash}"
        )

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

    if(GStreamer_USE_FRAMEWORK AND NOT DEFINED GSTREAMER_FRAMEWORK_PATH)
        set(GSTREAMER_FRAMEWORK_PATH "${GStreamer_ROOT_DIR}/../..")
        cmake_path(NORMAL_PATH GSTREAMER_FRAMEWORK_PATH)
    endif()

    set(GSTREAMER_INCLUDE_PATH "${GStreamer_ROOT_DIR}/include")
    set(GSTREAMER_LIB_PATH "${GStreamer_ROOT_DIR}/lib")
    set(GSTREAMER_PLUGIN_PATH "${GSTREAMER_LIB_PATH}/gstreamer-1.0")

    if(GStreamer_USE_FRAMEWORK)
        # Framework install: use the bundled pkg-config from the framework
        set(ENV{PKG_CONFIG} "${GStreamer_ROOT_DIR}/bin/pkg-config")
        set(PKG_CONFIG_EXECUTABLE "$ENV{PKG_CONFIG}" CACHE FILEPATH "pkg-config executable" FORCE)
        set(ENV{PKG_CONFIG_PATH} "${GSTREAMER_LIB_PATH}/pkgconfig:${GSTREAMER_PLUGIN_PATH}/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    else()
        # Auto-downloaded or Homebrew: restrict pkg-config to SDK paths only.
        _qgc_find_apple_pkg_config(PKG_CONFIG_EXECUTABLE)
        set(ENV{PKG_CONFIG_PATH} "")  # Clear to prevent host .pc files leaking in
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

        # Validate completeness of a previously expanded SDK (H2: stale cache detection)
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

    set(ENV{PKG_CONFIG_PATH} "")  # Clear to prevent host .pc files leaking in
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

set(GStreamer_ROOT_DIR "${GStreamer_ROOT_DIR}" CACHE PATH "GStreamer SDK root directory" FORCE)

if(GStreamer_USE_FRAMEWORK AND NOT EXISTS "${GSTREAMER_FRAMEWORK_PATH}")
    message(FATAL_ERROR "GStreamer: Could not locate framework at ${GSTREAMER_FRAMEWORK_PATH}")
endif()

# Sub-libraries used directly by QGC source code. Plugin transitive deps
# are resolved automatically from each plugin's .pc file in FindGStreamer.
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

set(GSTREAMER_APIS
    api_video
    api_gl
    api_rtsp
)

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

if(MACOS AND GStreamer_USE_FRAMEWORK)
    cmake_path(GET GSTREAMER_FRAMEWORK_PATH PARENT_PATH _gst_framework_parent)
    set(_saved_find_framework "${CMAKE_FIND_FRAMEWORK}")
    set(CMAKE_FIND_FRAMEWORK ONLY)
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
        message(STATUS "GStreamer: Found macOS framework at ${GSTREAMER_FRAMEWORK}")
    else()
        message(FATAL_ERROR "GStreamer: Could not locate GStreamer.framework")
    endif()
endif()

find_package(PkgConfig REQUIRED)

include(FindGStreamer)

if(ANDROID OR IOS)
    set(GStreamerMobile_FIND_COMPONENTS ${GSTREAMER_PLUGINS} ${GSTREAMER_APIS} mobile ca_certificates)
    if(GStreamerQGC_FIND_REQUIRED)
        set(GStreamerMobile_FIND_REQUIRED TRUE)
    endif()
    include(FindGStreamerMobile)
endif()

# Validate availability of core sub-libraries individually.
foreach(_comp_pair
    "Core;gstreamer-1.0"
    "Base;gstreamer-base-1.0"
    "Video;gstreamer-video-1.0"
    "Gl;gstreamer-gl-1.0"
    "GlPrototypes;gstreamer-gl-prototypes-1.0"
    "Rtsp;gstreamer-rtsp-1.0"
)
    list(GET _comp_pair 0 _comp_name)
    list(GET _comp_pair 1 _pc_name)
    pkg_check_modules(_GST_COMP_${_comp_name} QUIET ${_pc_name})
    if(_GST_COMP_${_comp_name}_FOUND)
        set(GStreamer_${_comp_name}_FOUND TRUE)
    else()
        set(GStreamer_${_comp_name}_FOUND FALSE)
    endif()
endforeach()

foreach(_gl_plat IN ITEMS egl wayland x11)
    string(SUBSTRING "${_gl_plat}" 0 1 _first)
    string(TOUPPER "${_first}" _first_upper)
    string(SUBSTRING "${_gl_plat}" 1 -1 _rest)
    string(TOUPPER "${_gl_plat}" _GL_UPPER)
    pkg_check_modules(_GST_GL_${_GL_UPPER} QUIET gstreamer-gl-${_gl_plat}-1.0)
    if(_GST_GL_${_GL_UPPER}_FOUND)
        set(GStreamer_Gl${_first_upper}${_rest}_FOUND TRUE)
    endif()
endforeach()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GStreamerQGC
    REQUIRED_VARS GStreamer_FOUND GSTREAMER_LIB_PATH GSTREAMER_PLUGIN_PATH
    VERSION_VAR GStreamer_VERSION
)
