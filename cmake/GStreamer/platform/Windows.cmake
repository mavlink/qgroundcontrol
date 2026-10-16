# Windows GStreamer SDK discovery — invoked by Orchestrator.cmake.

include_guard(GLOBAL)

# Sets <out_var> TRUE when <root_dir> holds a usable MSVC SDK (pkg-config, headers, plugins and gstreamer-1.0.pc).
function(_qgc_gst_windows_sdk_complete root_dir out_var)
    set(_required_paths "bin/pkg-config.exe" "include/gstreamer-1.0" "lib/gstreamer-1.0"
                        "lib/pkgconfig/gstreamer-1.0.pc"
    )

    set(_is_complete TRUE)
    foreach(_path IN LISTS _required_paths)
        if(NOT EXISTS "${root_dir}/${_path}")
            set(_is_complete FALSE)
            break()
        endif()
    endforeach()

    set(${out_var}
        "${_is_complete}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_windows_extract(<staging> <installer>)
# qgc_gst_acquire_sdk extractor: runs the SDK installer silently into <staging>, which becomes the SDK root, and checks
# the result.
function(_qgc_gst_windows_extract staging installer)
    cmake_path(NATIVE_PATH installer _installer_native)
    cmake_path(NATIVE_PATH staging _staging_native)
    cmake_path(GET staging PARENT_PATH _cache_dir)
    set(_installer_log "${_cache_dir}/installer.log")
    cmake_path(NATIVE_PATH _installer_log _installer_log_native)

    # The .exe is auto-run with /VERYSILENT below. Only its in-tree pin authenticates it: GStreamer_REQUIRE_CHECKSUM=OFF
    # also accepts upstream's same-origin .sha256sum sidecar, or no checksum at all.
    if(NOT _qgc_gst_archives_pinned)
        message(
            FATAL_ERROR "GStreamer: refusing to silently run the downloaded Windows installer "
                        "'${_installer_native}' because its checksum is not pinned in .github/build-config.json "
                        "(gstreamer.checksums), so its integrity was not verified. Pin it or install the SDK "
                        "manually and point GStreamer_ROOT_DIR at it."
        )
    endif()

    # /portable=1 (cerbero's Inno script, 1.28+) skips the uninstaller, the GSTREAMER_1_0_ROOT_* environment
    # variable, the InstallDir registry key, the VC++ redistributable and the Visual Studio templates. Without
    # it they would record <staging>, which is renamed once the install completes. /CURRENTUSER avoids
    # elevation; nothing is written outside <staging>.
    message(STATUS "Installing GStreamer SDK (silent)...")
    execute_process(
        COMMAND "${_installer_native}" /VERYSILENT /SUPPRESSMSGBOXES /SP- /NORESTART /CURRENTUSER /portable=1
                "/LOG=${_installer_log_native}" "/DIR=${_staging_native}"
        RESULT_VARIABLE _installer_rc
        ERROR_VARIABLE _installer_err
        TIMEOUT 600
    )
    if(NOT _installer_rc EQUAL 0)
        message(FATAL_ERROR "GStreamer installer failed (exit code: ${_installer_rc}).\n" "stderr: ${_installer_err}\n"
                            "See installer log: ${_installer_log}"
        )
    endif()

    _qgc_gst_windows_sdk_complete("${staging}" _complete)
    if(NOT _complete)
        message(FATAL_ERROR "GStreamer SDK installed into ${staging} but required files are missing.\n"
                            "See installer log: ${_installer_log}"
        )
    endif()
endfunction()

# _qgc_gst_windows_sdk_arch(<out_var>)
# Returns the SDK architecture (arm64 or x86_64) of the build's target: the MSVC compiler's target
# architecture when known, since a Ninja build from a vcvarsamd64_arm64 prompt reports the AMD64
# host as CMAKE_SYSTEM_PROCESSOR; otherwise the Visual Studio platform or the processor.
function(_qgc_gst_windows_sdk_arch out_var)
    if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
        set(_arch arm64)
    elseif(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
        set(_arch x86_64)
    elseif(CMAKE_GENERATOR_PLATFORM MATCHES "ARM64|aarch64"
           OR (NOT CMAKE_GENERATOR_PLATFORM AND CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|aarch64")
    )
        set(_arch arm64)
    else()
        set(_arch x86_64)
    endif()
    set(${out_var}
        "${_arch}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_windows_installed_sdk(<arch> <out_var>)
# Sets <out_var> to the installed <arch> (x86_64 or arm64) SDK named by the GSTREAMER_1_0_ROOT_<ARCH> environment
# variable, else by GSTREAMER_1_0_ROOT_MSVC_<ARCH> in MSVC builds, else found in a standard install location; "" when
# none exists.
function(_qgc_gst_windows_installed_sdk arch out_var)
    string(TOUPPER "${arch}" _env_arch)
    set(_root "")
    if(DEFINED ENV{GSTREAMER_1_0_ROOT_${_env_arch}} AND EXISTS "$ENV{GSTREAMER_1_0_ROOT_${_env_arch}}")
        set(_root "$ENV{GSTREAMER_1_0_ROOT_${_env_arch}}")
    elseif(
        MSVC
        AND DEFINED ENV{GSTREAMER_1_0_ROOT_MSVC_${_env_arch}}
        AND EXISTS "$ENV{GSTREAMER_1_0_ROOT_MSVC_${_env_arch}}"
    )
        set(_root "$ENV{GSTREAMER_1_0_ROOT_MSVC_${_env_arch}}")
    elseif(EXISTS "C:/Program Files/gstreamer/1.0/msvc_${arch}")
        set(_root "C:/Program Files/gstreamer/1.0/msvc_${arch}")
    elseif(EXISTS "C:/gstreamer/1.0/msvc_${arch}")
        set(_root "C:/gstreamer/1.0/msvc_${arch}")
    endif()
    set(${out_var}
        "${_root}"
        PARENT_SCOPE
    )
endfunction()

# _qgc_gst_discover_windows_sdk()
# Uses the SDK under GStreamer_ROOT_DIR, the GSTREAMER_1_0_ROOT_* environment or a standard install location when it is
# complete and recent enough, else downloads the official MSVC SDK for the target architecture. The SDK brings its own
# pkg-config.
# cmake-lint: disable=C0103
function(_qgc_gst_discover_windows_sdk)
    _qgc_gst_windows_sdk_arch(_gst_win_arch)
    if(_gst_win_arch STREQUAL "arm64")
        set(_gst_win_platform "windows_msvc_arm64")
    else()
        set(_gst_win_platform "windows_msvc_x64")
    endif()

    if(NOT GStreamer_ROOT_DIR)
        _qgc_gst_windows_installed_sdk(${_gst_win_arch} GStreamer_ROOT_DIR)
    endif()

    if(GStreamer_ROOT_DIR)
        _qgc_gst_windows_sdk_complete("${GStreamer_ROOT_DIR}" _gst_win_sdk_complete)
        _qgc_gst_header_version("${GStreamer_ROOT_DIR}/include" _gst_win_sdk_version)
        if(NOT _gst_win_sdk_complete)
            message(WARNING "Existing GStreamer SDK at ${GStreamer_ROOT_DIR} is incomplete "
                            "(missing devel/runtime artifacts). Falling back to auto-download."
            )
            unset(GStreamer_ROOT_DIR)
            unset(GStreamer_ROOT_DIR CACHE)
        elseif(NOT _gst_win_sdk_version OR _gst_win_sdk_version VERSION_LESS QGC_GST_REQUESTED_VERSION)
            message(WARNING "Existing GStreamer SDK at ${GStreamer_ROOT_DIR} has version "
                            "'${_gst_win_sdk_version}', but ${QGC_GST_REQUESTED_VERSION}+ is required. "
                            "Falling back to auto-download."
            )
            unset(GStreamer_ROOT_DIR)
            unset(GStreamer_ROOT_DIR CACHE)
        endif()
    endif()

    if(NOT GStreamer_ROOT_DIR)
        if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
            message(
                FATAL_ERROR "Automatic GStreamer download on Windows requires MSVC 64-bit (x64 or ARM64).\n"
                            "MinGW and Clang/Windows toolchains are NOT supported by the auto-download "
                            "path (the MSVC SDK ABI is incompatible).\n"
                            "Set GStreamer_ROOT_DIR to a complete SDK matching your toolchain/architecture."
            )
        endif()

        qgc_gst_acquire_sdk(
            PLATFORM
            ${_gst_win_platform}
            VERSION
            ${QGC_GST_REQUESTED_VERSION}
            CACHE_SUBDIR
            "gstreamer-win-${_gst_win_arch}-${QGC_GST_REQUESTED_VERSION}"
            ARCHIVES
            "gstreamer-${_gst_win_arch}.exe"
            CONTENT
            install
            EXTRACT_FUNCTION
            _qgc_gst_windows_extract
            CONTENT_DIR_OUT
            _gst_win_install_dir
        )
        set(GStreamer_ROOT_DIR "${_gst_win_install_dir}")
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")
    list(APPEND PKG_CONFIG_ARGN "--define-variable=prefix=${GStreamer_ROOT_DIR}"
         "--define-variable=libdir=${GSTREAMER_LIB_PATH}" "--define-variable=includedir=${GSTREAMER_INCLUDE_PATH}"
    )

    gstreamer_apply_pkgconfig_env(
        MODE
        SDK
        PKG_CONFIG_EXE
        "${GStreamer_ROOT_DIR}/bin/pkg-config.exe"
        LIBDIR
        "${GSTREAMER_LIB_PATH}/pkgconfig"
        "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        DONT_DEFINE_PREFIX
    )
    _qgc_gst_return_discovery()
endfunction()
