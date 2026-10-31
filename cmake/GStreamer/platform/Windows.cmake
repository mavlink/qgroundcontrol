# Windows GStreamer: SDK discovery, invoked by Orchestrator.cmake, and the installer qgc_gst_install_platform_sdk
# (Install.cmake) runs.

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
# complete and recent enough, else downloads the official MSVC SDK for the target architecture. An SDK the user named
# in GStreamer_ROOT_DIR that is incomplete or too old fails configure; one found elsewhere is replaced by a download.
# The SDK brings its own pkg-config.
# cmake-lint: disable=C0103
function(_qgc_gst_discover_windows_sdk)
    _qgc_gst_windows_sdk_arch(_gst_win_arch)

    set(_gst_win_user_root FALSE)
    if(GStreamer_ROOT_DIR AND NOT GStreamer_AUTO_DOWNLOADED)
        set(_gst_win_user_root TRUE)
    elseif(NOT GStreamer_ROOT_DIR)
        _qgc_gst_windows_installed_sdk(${_gst_win_arch} GStreamer_ROOT_DIR)
    endif()

    if(GStreamer_ROOT_DIR)
        _qgc_gst_windows_sdk_complete("${GStreamer_ROOT_DIR}" _gst_win_sdk_complete)
        _qgc_gst_header_version("${GStreamer_ROOT_DIR}/include" _gst_win_sdk_version)
        set(_gst_win_problem "")
        if(NOT _gst_win_sdk_complete)
            set(_gst_win_problem "is incomplete (missing devel or runtime files)")
        elseif(NOT _gst_win_sdk_version OR _gst_win_sdk_version VERSION_LESS QGC_GST_REQUESTED_VERSION)
            set(_gst_win_problem
                "has version '${_gst_win_sdk_version}', but ${QGC_GST_REQUESTED_VERSION} or later is required"
            )
        endif()
        if(_gst_win_problem AND _gst_win_user_root)
            message(
                FATAL_ERROR
                    "GStreamer: the SDK at ${GStreamer_ROOT_DIR} (GStreamer_ROOT_DIR) ${_gst_win_problem}. Point "
                    "GStreamer_ROOT_DIR at a complete SDK of that version, or unset it to download one."
            )
        elseif(_gst_win_problem)
            message(WARNING "GStreamer: the SDK at ${GStreamer_ROOT_DIR} ${_gst_win_problem}; downloading one instead.")
            unset(GStreamer_ROOT_DIR)
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

    qgc_gst_create_layout_target(SDK_ROOT "${GStreamer_ROOT_DIR}")
    list(APPEND PKG_CONFIG_ARGN "--define-variable=prefix=${GStreamer_ROOT_DIR}"
         "--define-variable=libdir=${GSTREAMER_LIB_PATH}" "--define-variable=includedir=${GSTREAMER_INCLUDE_PATH}"
    )

    qgc_gst_apply_pkgconfig_env(
        MODE SDK
        PKG_CONFIG_EXE "${GStreamer_ROOT_DIR}/bin/pkg-config.exe"
        LIBDIR "${GSTREAMER_LIB_PATH}/pkgconfig" "${GSTREAMER_PLUGIN_PATH}/pkgconfig"
        DONT_DEFINE_PREFIX
    )
    _qgc_gst_return_discovery()
endfunction()

# Windows: the plugins, GIO modules, libproxy backends and libexec helpers, the SDK DLLs they and QGC load, and the
# notice. gioopenssl verifies against the Windows certificate stores, so the SDK's CA bundle stays out.
function(qgc_gst_install_windows_sdk app_name plugin_dest_out)
    set(_plugin_dest "${CMAKE_INSTALL_LIBDIR}/gstreamer-1.0")
    _qgc_gst_begin_runtime_closure()
    qgc_gst_install_plugins("${GSTREAMER_PLUGIN_PATH}" "${_plugin_dest}" "gst" "dll")
    qgc_gst_install_gio_modules("${GSTREAMER_GIO_MODULE_PATH}" "${CMAKE_INSTALL_LIBDIR}/gio/modules" "dll")
    # giolibproxy.dll loads proxy-1.dll, which loads its backends from lib/libproxy; startup only puts the app's bin
    # directory on PATH, so they go there too.
    file(GLOB _gst_libproxy_backends "${GStreamer_ROOT_DIR}/lib/libproxy/*.dll")
    _qgc_gst_install_files("${CMAKE_INSTALL_BINDIR}" ${_gst_libproxy_backends})
    file(GLOB _gst_helpers "${GSTREAMER_PLUGIN_SCANNER_PATH}/*.exe")
    _qgc_gst_install_files("${CMAKE_INSTALL_LIBEXECDIR}/gstreamer-1.0" PROGRAMS ${_gst_helpers})
    _qgc_gst_install_runtime_closure("${CMAKE_INSTALL_BINDIR}")
    _qgc_gst_install_notice(share/licenses/gstreamer)
    set(${plugin_dest_out}
        "${_plugin_dest}"
        PARENT_SCOPE
    )
endfunction()
