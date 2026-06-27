# Windows GStreamer SDK discovery — invoked by Orchestrator.cmake.

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
        # A bundled second pkg-config.exe would yield a non-canonical root via
        # element 0 — pick the shortest path so the canonical SDK root wins.
        set(_first_pkg_config "")
        set(_shortest_depth -1)
        foreach(_candidate IN LISTS _pkg_config_files)
            string(REGEX MATCHALL "/" _candidate_slashes "${_candidate}")
            list(LENGTH _candidate_slashes _candidate_depth)
            # Lower-case the equal-depth tiebreak so it stays deterministic on
            # Windows' case-insensitive filesystem.
            string(TOLOWER "${_candidate}" _candidate_lc)
            string(TOLOWER "${_first_pkg_config}" _first_pkg_config_lc)
            if(_shortest_depth EQUAL -1
               OR _candidate_depth LESS _shortest_depth
               OR (_candidate_depth EQUAL _shortest_depth AND _candidate_lc STRLESS "${_first_pkg_config_lc}"))
                set(_first_pkg_config "${_candidate}")
                set(_shortest_depth ${_candidate_depth})
            endif()
        endforeach()
        cmake_path(GET _first_pkg_config PARENT_PATH _bin_dir)
        cmake_path(GET _bin_dir PARENT_PATH _found_root)
        set(${OUT_VAR} "${_found_root}" PARENT_SCOPE)
    else()
        set(${OUT_VAR} "" PARENT_SCOPE)
    endif()
endfunction()

macro(_qgc_discover_windows_sdk)
    if(CMAKE_GENERATOR_PLATFORM MATCHES "ARM64|aarch64" OR
       (NOT CMAKE_GENERATOR_PLATFORM AND CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64"))
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
                "MinGW and Clang/Windows toolchains are NOT supported by the auto-download "
                "path (the MSVC SDK ABI is incompatible).\n"
                "Set GStreamer_ROOT_DIR to a complete SDK matching your toolchain/architecture.")
        endif()

        gstreamer_resolve_or_download_sdk(
            PLATFORM     ${_gst_win_platform}
            CACHE_SUBDIR "gstreamer-win-${_gst_win_arch}-${GStreamer_FIND_VERSION}"
            FILENAME_PRIMARY "gstreamer-${_gst_win_arch}.exe"
            CACHE_DIR_OUT _gst_win_cache_dir
            ARCHIVE_OUT   _gst_win_exe
        )
        set(_gst_win_extracted "${_gst_win_cache_dir}/sdk")

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

            # The .exe is auto-run with /VERYSILENT below; with checksum verification
            # off it may be unverified — refuse to silently execute it.
            if(NOT GStreamer_REQUIRE_CHECKSUM)
                message(FATAL_ERROR
                    "GStreamer: refusing to silently run the downloaded Windows installer "
                    "'${_gst_win_exe_native}' because GStreamer_REQUIRE_CHECKSUM is OFF, so its "
                    "integrity was not verified. Re-enable checksum verification "
                    "(-DGStreamer_REQUIRE_CHECKSUM=ON) or install the SDK manually and point "
                    "GStreamer_ROOT_DIR at it.")
            endif()

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

            set(_gst_win_post_complete FALSE)
            if(_gst_win_root)
                _qgc_windows_sdk_complete("${_gst_win_root}" _gst_win_post_complete)
            endif()
            if(NOT _gst_win_post_complete)
                file(REMOVE_RECURSE "${_gst_win_extracted}")
                message(FATAL_ERROR "GStreamer SDK extracted but required files are missing.\n"
                    "Delete ${_gst_win_cache_dir} and re-run cmake to retry.")
            endif()
        endif()

        set(GStreamer_ROOT_DIR "${_gst_win_root}")
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    gstreamer_create_layout_target(
        SDK_ROOT "${GStreamer_ROOT_DIR}"
        TYPE FLAT
    )
    # Strip any prior define-variable entries (from a cached PKG_CONFIG_ARGN on
    # reconfigure) before re-deriving them, so values can't double-apply or go stale.
    list(FILTER PKG_CONFIG_ARGN EXCLUDE REGEX "^--define-variable=(prefix|libdir|includedir)=")
    list(APPEND PKG_CONFIG_ARGN
        "--define-variable=prefix=${GStreamer_ROOT_DIR}"
        "--define-variable=libdir=${GSTREAMER_LIB_PATH}"
        "--define-variable=includedir=${GSTREAMER_INCLUDE_PATH}"
    )

    gstreamer_apply_pkgconfig_env(
        MODE SDK
        PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/bin/pkg-config.exe"
        LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        DONT_DEFINE_PREFIX
    )
endmacro()
