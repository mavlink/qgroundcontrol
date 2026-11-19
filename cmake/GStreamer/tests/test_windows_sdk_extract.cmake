cmake_minimum_required(VERSION 3.25)
# The Windows SDK installer is run silently into the cache once it matched its pinned checksum. A stand-in installer
# script is pre-seeded in the cache, so nothing is downloaded.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Acquire)

if(CMAKE_HOST_WIN32)
    message(STATUS "QGC_TEST_SKIPPED: the stand-in installer is a shell script")
    return()
endif()

set(_version 9.9.4)
qgc_test_sandbox(_sandbox windows-sdk-extract)
set(CPM_SOURCE_CACHE "${_sandbox}/cache")
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
set(QGC_BUILD_CONFIG_CONTENT "{\"gstreamer\":{\"checksums\":{\"${_version}\":{\"windows_msvc_x64\":\"${_hash}\"}}}}")

set(QGC_GST_REQUESTED_VERSION ${_version})
_qgc_gst_acquire_windows_sdk(x86_64 _install_dir)
_qgc_gst_windows_sdk_complete("${_install_dir}" _complete)
qgc_test_assert_streq("installer ran" TRUE "${_complete}")
qgc_test_pass("pinned Windows installer installs the SDK into the cache")
