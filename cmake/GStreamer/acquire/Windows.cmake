# Windows GStreamer SDK acquisition: the official MSVC installer, run silently into the cache.
# Included by Acquire.cmake, whose qgc_gst_acquire_sdk does the download, caching and locking.

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
            FATAL_ERROR
                "GStreamer: refusing to silently run the downloaded Windows installer "
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
        message(
            FATAL_ERROR "GStreamer installer failed (exit code: ${_installer_rc}).\n" "stderr: ${_installer_err}\n"
                        "See installer log: ${_installer_log}"
        )
    endif()

    _qgc_gst_windows_sdk_complete("${staging}" _complete)
    if(NOT _complete)
        message(
            FATAL_ERROR "GStreamer SDK installed into ${staging} but required files are missing.\n"
                        "See installer log: ${_installer_log}"
        )
    endif()
endfunction()

# _qgc_gst_acquire_windows_sdk(<arch> <out_root>)
# Downloads and silently installs the official MSVC SDK for <arch> (x86_64 or arm64); sets <out_root> to its root.
function(_qgc_gst_acquire_windows_sdk arch out_root)
    if(arch STREQUAL "arm64")
        set(_platform windows_msvc_arm64)
    else()
        set(_platform windows_msvc_x64)
    endif()
    qgc_gst_acquire_sdk(
        PLATFORM ${_platform}
        VERSION ${QGC_GST_REQUESTED_VERSION}
        CACHE_SUBDIR "gstreamer-win-${arch}-${QGC_GST_REQUESTED_VERSION}"
        ARCHIVES "gstreamer-${arch}.exe"
        CONTENT install
        EXTRACT_FUNCTION _qgc_gst_windows_extract
        CONTENT_DIR_OUT _root
    )
    set(${out_root}
        "${_root}"
        PARENT_SCOPE
    )
endfunction()
