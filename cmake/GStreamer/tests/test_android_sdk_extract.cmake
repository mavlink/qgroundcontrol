cmake_minimum_required(VERSION 3.25)
# Runs Android SDK discovery in script mode against a pinned stand-in for the universal tarball:
# each ABI build must extract exactly its own tree (x86 must not pick up x86_64) and the trees of
# ABIs built from one cache coexist, as with Qt's multi-ABI sub-builds.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Download)
include(Layout)
include(PkgConfig)
include(Platform)
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Android.cmake")

# Discovery looks up a Homebrew pkg-config on macOS hosts; that lookup is not under test here.
function(_qgc_gst_find_apple_pkg_config out_var)

endfunction()

qgc_test_sandbox(_sandbox android-sdk-extract)
set(CPM_SOURCE_CACHE "${_sandbox}/cache")
set(QGC_GST_REQUESTED_VERSION 9.9.2)
set(_cache_dir "${CPM_SOURCE_CACHE}/gstreamer-android-${QGC_GST_REQUESTED_VERSION}")
set(_archive "${_cache_dir}/gstreamer-android-${QGC_GST_REQUESTED_VERSION}.tar.xz")

set(_abi_dirs armv7 arm64 x86 x86_64)
foreach(_abi_dir IN LISTS _abi_dirs)
    file(WRITE "${_sandbox}/src/${_abi_dir}/lib/pkgconfig/gstreamer-1.0.pc" "Version: ${QGC_GST_REQUESTED_VERSION}\n")
    file(WRITE "${_sandbox}/src/${_abi_dir}/include/abi.txt" "${_abi_dir}")
    file(WRITE "${_sandbox}/src/${_abi_dir}/lib/gstreamer-1.0/libgstcoreelements.a" "")
endforeach()
file(MAKE_DIRECTORY "${_cache_dir}")
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cJf "${_archive}" ${_abi_dirs}
                WORKING_DIRECTORY "${_sandbox}/src" COMMAND_ERROR_IS_FATAL ANY
)
file(SHA256 "${_archive}" _hash)
set(QGC_BUILD_CONFIG_CONTENT
    "{\"gstreamer\":{\"checksums\":{\"${QGC_GST_REQUESTED_VERSION}\":{\"android\":\"${_hash}\"}}}}"
)

foreach(_case IN ITEMS "x86|x86" "x86_64|x86_64" "arm64-v8a|arm64")
    qgc_test_split("${_case}" CMAKE_ANDROID_ARCH_ABI _abi_dir)
    set(GStreamer_ROOT_DIR "")
    set(GStreamer_AUTO_DOWNLOADED FALSE)
    _qgc_gst_discover_android_sdk()

    set(_root "${_cache_dir}/${_abi_dir}/${_abi_dir}")
    qgc_test_assert_streq("${CMAKE_ANDROID_ARCH_ABI} root" "${_root}" "${GStreamer_ROOT_DIR}")
    qgc_test_assert_streq("${CMAKE_ANDROID_ARCH_ABI} lib path" "${_root}/lib" "${GSTREAMER_LIB_PATH}")
    qgc_test_assert_streq("${CMAKE_ANDROID_ARCH_ABI} auto-downloaded" "TRUE" "${GStreamer_AUTO_DOWNLOADED}")
    file(READ "${_root}/include/abi.txt" _extracted_abi)
    qgc_test_assert_streq("${CMAKE_ANDROID_ARCH_ABI} tree" "${_abi_dir}" "${_extracted_abi}")
endforeach()

# Only the ABIs that were built are extracted, and each one exactly once.
file(
    GLOB _published
    RELATIVE "${_cache_dir}"
    "${_cache_dir}/*/.complete"
)
list(SORT _published)
qgc_test_assert_streq("published trees" "arm64/.complete;x86/.complete;x86_64/.complete" "${_published}")
qgc_test_pass("Android SDK extraction per ABI")
