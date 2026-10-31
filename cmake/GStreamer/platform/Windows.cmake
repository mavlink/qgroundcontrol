# Windows GStreamer SDK discovery — invoked by Orchestrator.cmake.

include_guard(GLOBAL)

# _qgc_gst_windows_sdk_arch(<out_var>)
# Returns the SDK architecture (arm64 or x86_64) of the build's target: the MSVC compiler's target
# architecture when known, since a Ninja build from a vcvarsamd64_arm64 prompt reports the AMD64
# host as CMAKE_SYSTEM_PROCESSOR; otherwise the Visual Studio platform or the processor.
function(_qgc_gst_windows_sdk_arch out_var)
    if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "ARM64")
        set(_arch arm64)
    elseif(CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64")
        set(_arch x86_64)
    elseif(
        CMAKE_GENERATOR_PLATFORM MATCHES "ARM64|aarch64" OR (NOT CMAKE_GENERATOR_PLATFORM AND CMAKE_SYSTEM_PROCESSOR
                                                                                              MATCHES "ARM64|aarch64")
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
# none exists. An installation in a path with spaces (the installer's default, C:/Program Files) is skipped with a
# warning, since pkg-config cannot pass such paths on (_qgc_gst_require_no_spaces).
function(_qgc_gst_windows_installed_sdk arch out_var)
    string(TOUPPER "${arch}" _env_arch)
    set(_candidates "$ENV{GSTREAMER_1_0_ROOT_${_env_arch}}")
    if(MSVC)
        list(APPEND _candidates "$ENV{GSTREAMER_1_0_ROOT_MSVC_${_env_arch}}")
    endif()
    list(APPEND _candidates "C:/Program Files/gstreamer/1.0/msvc_${arch}" "C:/gstreamer/1.0/msvc_${arch}")
    set(_root "")
    foreach(_candidate IN LISTS _candidates)
        if(NOT _candidate OR NOT EXISTS "${_candidate}")
            continue()
        endif()
        cmake_path(SET _candidate "${_candidate}")
        if(_candidate MATCHES " ")
            message(
                WARNING "GStreamer: skipping the SDK installed at '${_candidate}': pkg-config cannot pass on a "
                        "path with spaces. Install it to C:/gstreamer/1.0/msvc_${arch} to use it."
            )
            continue()
        endif()
        set(_root "${_candidate}")
        break()
    endforeach()
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

    if(NOT GStreamer_ROOT_DIR)
        _qgc_gst_windows_installed_sdk(${_gst_win_arch} GStreamer_ROOT_DIR)
    endif()

    if(GStreamer_ROOT_DIR)
        _qgc_gst_windows_sdk_complete("${GStreamer_ROOT_DIR}" _gst_win_sdk_complete)
        _qgc_gst_header_version("${GStreamer_ROOT_DIR}/include" _gst_win_sdk_version)
        if(NOT _gst_win_sdk_complete)
            message(
                WARNING "Existing GStreamer SDK at ${GStreamer_ROOT_DIR} is incomplete "
                        "(missing devel/runtime artifacts). Falling back to auto-download."
            )
            unset(GStreamer_ROOT_DIR)
            unset(GStreamer_ROOT_DIR CACHE)
        elseif(NOT _gst_win_sdk_version OR _gst_win_sdk_version VERSION_LESS QGC_GST_REQUESTED_VERSION)
            message(
                WARNING
                    "Existing GStreamer SDK at ${GStreamer_ROOT_DIR} has version "
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
                FATAL_ERROR
                    "Automatic GStreamer download on Windows requires MSVC 64-bit (x64 or ARM64).\n"
                    "MinGW and Clang/Windows toolchains are NOT supported by the auto-download "
                    "path (the MSVC SDK ABI is incompatible).\n"
                    "Set GStreamer_ROOT_DIR to a complete SDK matching your toolchain/architecture."
            )
        endif()

        _qgc_gst_space_free_download_cache()
        _qgc_gst_acquire_windows_sdk(${_gst_win_arch} GStreamer_ROOT_DIR)
        set(GStreamer_AUTO_DOWNLOADED TRUE)
    endif()

    gstreamer_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")
    list(APPEND PKG_CONFIG_ARGN "--define-variable=prefix=${GStreamer_ROOT_DIR}"
         "--define-variable=libdir=${GSTREAMER_LIB_PATH}" "--define-variable=includedir=${GSTREAMER_INCLUDE_PATH}"
    )

    gstreamer_apply_pkgconfig_env(
        MODE SDK
        PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/bin/pkg-config.exe"
        LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        DONT_DEFINE_PREFIX
    )
    _qgc_gst_return_discovery()
endfunction()
