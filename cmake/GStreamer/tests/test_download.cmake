cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/.." "${CMAKE_CURRENT_LIST_DIR}/../../modules")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Packages)

qgc_gst_get_package_url(android 1.28.2 _u_android)
qgc_test_assert_streq(
    "android url"
    "https://gstreamer.freedesktop.org/data/pkg/android/1.28.2/gstreamer-1.0-android-universal-1.28.2.tar.xz"
    "${_u_android}"
)

qgc_gst_get_package_url(ios 1.28.8 _u_ios)
qgc_test_assert_streq(
    "ios url" "https://gstreamer.freedesktop.org/data/pkg/ios/1.28.8/gstreamer-1.28.8-xcframework.tar.xz" "${_u_ios}"
)

qgc_gst_get_package_url(macos 1.28.2 _u_mac)
qgc_test_assert_streq(
    "macos url" "https://gstreamer.freedesktop.org/data/pkg/macos/1.28.2/gstreamer-1.0-1.28.2-universal.pkg"
    "${_u_mac}"
)

qgc_gst_get_package_url(macos_devel 1.28.2 _u_macd)
qgc_test_assert_streq(
    "macos_devel url"
    "https://gstreamer.freedesktop.org/data/pkg/macos/1.28.2/gstreamer-1.0-devel-1.28.2-universal.pkg" "${_u_macd}"
)

qgc_gst_get_package_url(windows_msvc_x64 1.28.2 _u_winx)
qgc_test_assert_streq(
    "windows x64 url"
    "https://gstreamer.freedesktop.org/data/pkg/windows/1.28.2/msvc/gstreamer-1.0-msvc-x86_64-1.28.2.exe" "${_u_winx}"
)

qgc_gst_get_package_url(windows_msvc_arm64 1.28.2 _u_wina)
qgc_test_assert_streq(
    "windows arm64 url"
    "https://gstreamer.freedesktop.org/data/pkg/windows/1.28.2/msvc/gstreamer-1.0-msvc-arm64-1.28.2.exe" "${_u_wina}"
)

qgc_test_pass("get_package_url known platforms")

set(_s3_base "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer")

qgc_gst_get_s3_mirror_url(android 1.28.2 _s3_android)
qgc_test_assert_streq("s3 android" "${_s3_base}/android/gstreamer-1.0-android-universal-1.28.2.tar.xz" "${_s3_android}")

qgc_gst_get_s3_mirror_url(macos 1.28.2 _s3_mac)
qgc_test_assert_streq("s3 macos" "${_s3_base}/macos/gstreamer-1.0-1.28.2-universal.pkg" "${_s3_mac}")

qgc_gst_get_s3_mirror_url(windows_msvc_x64 1.28.2 _s3_win)
qgc_test_assert_streq("s3 windows" "${_s3_base}/windows/gstreamer-1.0-msvc-x86_64-1.28.2.exe" "${_s3_win}")

qgc_gst_get_s3_mirror_url(macos_devel 1.28.2 _s3_macd)
qgc_test_assert_streq("s3 macos_devel" "${_s3_base}/macos/gstreamer-1.0-devel-1.28.2-universal.pkg" "${_s3_macd}")

qgc_test_pass("get_s3_mirror_url routing")

# Records each package download's URLS, EXPECTED_HASH and TIMEOUT instead of downloading.
function(qgc_resilient_download)
    cmake_parse_arguments(
        PARSE_ARGV 0 ARG "" "FILENAME;DESTINATION_DIR;RESULT_VAR;TIMEOUT;EXPECTED_HASH;LOG_TAG;FAILURE_HINT" "URLS"
    )
    set_property(GLOBAL PROPERTY _qgc_test_urls "${ARG_URLS}")
    set_property(GLOBAL PROPERTY _qgc_test_hash "${ARG_EXPECTED_HASH}")
    set_property(GLOBAL PROPERTY _qgc_test_timeout "${ARG_TIMEOUT}")
    set(${ARG_RESULT_VAR}
        "${ARG_DESTINATION_DIR}/${ARG_FILENAME}"
        PARENT_SCOPE
    )
endfunction()

# A package without a pinned checksum fails configure instead of being downloaded.
if(TEST_CASE STREQUAL "unpinned")
    set(QGC_BUILD_CONFIG_CONTENT "{}")
    _qgc_gst_download_archive(android 9.1.2 package.tar.xz "${CMAKE_CURRENT_BINARY_DIR}")
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

# Upstream serves every package, the mirror only when upstream fails; both are held to the pinned checksum.
string(SHA256 _pin "pinned")
set(QGC_BUILD_CONFIG_CONTENT "{\"gstreamer\":{\"checksums\":{\"9.1.1\":{\"android\":\"${_pin}\"}}}}")
qgc_gst_get_package_url(android 9.1.1 _upstream)
qgc_gst_get_s3_mirror_url(android 9.1.1 _mirror)
_qgc_gst_download_archive(android 9.1.1 package.tar.xz "${CMAKE_CURRENT_BINARY_DIR}")
get_property(_urls GLOBAL PROPERTY _qgc_test_urls)
get_property(_verified_hash GLOBAL PROPERTY _qgc_test_hash)
qgc_test_assert_streq("upstream first" "${_upstream};${_mirror}" "${_urls}")
qgc_test_assert_streq("verified against the pin" "SHA256=${_pin}" "${_verified_hash}")
qgc_test_pass("package comes from upstream, verified against its pin")

# QGC_DOWNLOAD_TIMEOUT|expected wall-clock cap: a slow link can raise the SDK's cap but nothing lowers it.
foreach(_case IN ITEMS "|1200" "300|1200" "7200|7200")
    qgc_test_split("${_case}" QGC_DOWNLOAD_TIMEOUT _expected_timeout)
    _qgc_gst_download_archive(android 9.1.1 package.tar.xz "${CMAKE_CURRENT_BINARY_DIR}")
    get_property(_timeout GLOBAL PROPERTY _qgc_test_timeout)
    qgc_test_assert_streq("cap with QGC_DOWNLOAD_TIMEOUT=${QGC_DOWNLOAD_TIMEOUT}" "${_expected_timeout}" "${_timeout}")
endforeach()
qgc_test_pass("QGC_DOWNLOAD_TIMEOUT raises the download's wall-clock cap")
