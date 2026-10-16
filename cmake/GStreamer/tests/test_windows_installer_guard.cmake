cmake_minimum_required(VERSION 3.25)
# The Windows SDK installer runs silently only when it matched its pinned checksum, also with
# GStreamer_REQUIRE_CHECKSUM=OFF. A stand-in installer script is pre-seeded in the cache, so nothing is downloaded.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Download)
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Windows.cmake")

# Unpinned, the installer is verified against a seeded same-origin sidecar instead, which must not be enough.
if(TEST_CASE STREQUAL "unpinned")
    set(_version 9.9.5)
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
else()
    if(CMAKE_HOST_WIN32)
        message(STATUS "QGC_TEST_SKIPPED: the stand-in installer is a shell script")
        return()
    endif()
    set(_version 9.9.4)
endif()

qgc_test_sandbox(_sandbox windows-installer-guard-${_version})
set(CPM_SOURCE_CACHE "${_sandbox}/cache")
set(GStreamer_REQUIRE_CHECKSUM OFF)
set(_cache_subdir "gstreamer-win-x86_64-${_version}")
set(_installer "${CPM_SOURCE_CACHE}/${_cache_subdir}/gstreamer-x86_64.exe")

# Installs a minimal complete SDK into the /DIR=<dir> it is given.
file(
    WRITE "${_sandbox}/gstreamer-x86_64.exe"
    "#!/bin/sh\n" "for arg in \"$@\"; do case \"$arg\" in /DIR=*) dir=\"\${arg#/DIR=}\" ;; esac; done\n"
    "mkdir -p \"$dir/bin\" \"$dir/include/gstreamer-1.0\" \"$dir/lib/gstreamer-1.0\" \"$dir/lib/pkgconfig\"\n"
    ": > \"$dir/bin/pkg-config.exe\"\n" ": > \"$dir/lib/pkgconfig/gstreamer-1.0.pc\"\n"
)
file(
    COPY "${_sandbox}/gstreamer-x86_64.exe"
    DESTINATION "${CPM_SOURCE_CACHE}/${_cache_subdir}"
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
)
file(SHA256 "${_installer}" _hash)
if(TEST_CASE STREQUAL "unpinned")
    set(QGC_BUILD_CONFIG_CONTENT "{}")
    gstreamer_get_package_url(windows_msvc_x64 ${_version} _url)
    string(SHA256 _url_hash "${_url}.sha256sum")
    file(WRITE "${CMAKE_BINARY_DIR}/_deps/checksums/${_url_hash}.sha256sum" "${_hash}  gstreamer.exe\n")
else()
    set(QGC_BUILD_CONFIG_CONTENT
        "{\"gstreamer\":{\"checksums\":{\"${_version}\":{\"windows_msvc_x64\":\"${_hash}\"}}}}"
    )
endif()

qgc_gst_acquire_sdk(
    PLATFORM
    windows_msvc_x64
    VERSION
    ${_version}
    CACHE_SUBDIR
    "${_cache_subdir}"
    ARCHIVES
    gstreamer-x86_64.exe
    CONTENT
    install
    EXTRACT_FUNCTION
    _qgc_gst_windows_extract
    CONTENT_DIR_OUT
    _install_dir
)
if(DEFINED TEST_CASE)
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
endif()
_qgc_gst_windows_sdk_complete("${_install_dir}" _complete)
qgc_test_assert_streq("pinned installer ran" TRUE "${_complete}")
qgc_test_pass("pinned Windows installer runs with GStreamer_REQUIRE_CHECKSUM=OFF")
