cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/.." "${CMAKE_CURRENT_LIST_DIR}/../../modules")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Download)

gstreamer_get_package_url(android 1.28.2 _u_android)
qgc_test_assert_streq(
    "android url"
    "https://gstreamer.freedesktop.org/data/pkg/android/1.28.2/gstreamer-1.0-android-universal-1.28.2.tar.xz"
    "${_u_android}"
)

gstreamer_get_package_url(ios 1.28.2 _u_ios)
qgc_test_assert_streq(
    "ios url" "https://gstreamer.freedesktop.org/data/pkg/ios/1.28.2/gstreamer-1.0-devel-1.28.2-ios-universal.pkg"
    "${_u_ios}"
)

gstreamer_get_package_url(macos 1.28.2 _u_mac)
qgc_test_assert_streq("macos url"
                      "https://gstreamer.freedesktop.org/data/pkg/macos/1.28.2/gstreamer-1.0-1.28.2-universal.pkg"
                      "${_u_mac}"
)

gstreamer_get_package_url(macos_devel 1.28.2 _u_macd)
qgc_test_assert_streq("macos_devel url"
                      "https://gstreamer.freedesktop.org/data/pkg/macos/1.28.2/gstreamer-1.0-devel-1.28.2-universal.pkg"
                      "${_u_macd}"
)

gstreamer_get_package_url(windows_msvc_x64 1.28.2 _u_winx)
qgc_test_assert_streq(
    "windows x64 url"
    "https://gstreamer.freedesktop.org/data/pkg/windows/1.28.2/msvc/gstreamer-1.0-msvc-x86_64-1.28.2.exe" "${_u_winx}"
)

gstreamer_get_package_url(windows_msvc_arm64 1.28.2 _u_wina)
qgc_test_assert_streq(
    "windows arm64 url"
    "https://gstreamer.freedesktop.org/data/pkg/windows/1.28.2/msvc/gstreamer-1.0-msvc-arm64-1.28.2.exe" "${_u_wina}"
)

qgc_test_pass("get_package_url known platforms")

set(_s3_base "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer")

gstreamer_get_s3_mirror_url(android 1.28.2 _s3_android)
qgc_test_assert_streq("s3 android" "${_s3_base}/android/gstreamer-1.0-android-universal-1.28.2.tar.xz" "${_s3_android}")

gstreamer_get_s3_mirror_url(macos 1.28.2 _s3_mac)
qgc_test_assert_streq("s3 macos" "${_s3_base}/macos/gstreamer-1.0-1.28.2-universal.pkg" "${_s3_mac}")

gstreamer_get_s3_mirror_url(windows_msvc_x64 1.28.2 _s3_win)
qgc_test_assert_streq("s3 windows" "${_s3_base}/windows/gstreamer-1.0-msvc-x86_64-1.28.2.exe" "${_s3_win}")

gstreamer_get_s3_mirror_url(macos_devel 1.28.2 _s3_macd)
qgc_test_assert_streq("s3 macos_devel" "${_s3_base}/macos/gstreamer-1.0-devel-1.28.2-universal.pkg" "${_s3_macd}")

qgc_test_pass("get_s3_mirror_url routing")

# Records each package download's URLS and EXPECTED_HASH instead of downloading; a .sha256sum sidecar request is
# answered with the sidecar of <_sidecar_hash>.
function(qgc_resilient_download)
    set(_one_value_keywords
        FILENAME
        DESTINATION_DIR
        RESULT_VAR
        TIMEOUT
        INACTIVITY_TIMEOUT
        RETRIES
        RETRY_BACKOFF
        EXPECTED_HASH
        LOG_TAG
        FAILURE_HINT
    )
    cmake_parse_arguments(PARSE_ARGV 0 ARG "ALLOW_FAILURE" "${_one_value_keywords}" "URLS")
    if(ARG_FILENAME MATCHES "\\.sha256sum$")
        file(WRITE "${ARG_DESTINATION_DIR}/${ARG_FILENAME}" "${_sidecar_hash}  package\n")
    else()
        set_property(GLOBAL PROPERTY _qgc_test_urls "${ARG_URLS}")
        set_property(GLOBAL PROPERTY _qgc_test_hash "${ARG_EXPECTED_HASH}")
    endif()
    set(${ARG_RESULT_VAR}
        "${ARG_DESTINATION_DIR}/${ARG_FILENAME}"
        PARENT_SCOPE
    )
endfunction()

# Downloads the android <version> package with the stub above; sets _urls and _verified_hash in the caller.
function(_qgc_test_download version)
    _qgc_gst_download_archive(android ${version} package.tar.xz "${CMAKE_CURRENT_BINARY_DIR}" _hash)
    get_property(_urls GLOBAL PROPERTY _qgc_test_urls)
    get_property(_verified GLOBAL PROPERTY _qgc_test_hash)
    qgc_test_assert_streq("${version} reports the hash it verified against" "${_verified}" "${_hash}")
    set(_urls
        "${_urls}"
        PARENT_SCOPE
    )
    set(_verified_hash
        "${_verified}"
        PARENT_SCOPE
    )
endfunction()

# Upstream serves every package, the mirror only when upstream fails; both are held to the same checksum.
string(SHA256 _pin "pinned")
set(QGC_BUILD_CONFIG_CONTENT "{\"gstreamer\":{\"checksums\":{\"9.1.1\":{\"android\":\"${_pin}\"}}}}")
gstreamer_get_package_url(android 9.1.1 _upstream)
gstreamer_get_s3_mirror_url(android 9.1.1 _mirror)
_qgc_test_download(9.1.1)
qgc_test_assert_streq("pinned: upstream first" "${_upstream};${_mirror}" "${_urls}")
qgc_test_assert_streq("pinned: verified against the pin" "SHA256=${_pin}" "${_verified_hash}")
qgc_test_pass("pinned package comes from upstream, verified against its pin")

# An unpinned package is verified against upstream's .sha256sum sidecar.
string(SHA256 _sidecar_hash "sidecar")
set(QGC_BUILD_CONFIG_CONTENT "{}")
set(GStreamer_REQUIRE_CHECKSUM OFF)
gstreamer_get_package_url(android 9.1.2 _upstream)
gstreamer_get_s3_mirror_url(android 9.1.2 _mirror)
_qgc_test_download(9.1.2)
qgc_test_assert_streq("unpinned: upstream first" "${_upstream};${_mirror}" "${_urls}")
qgc_test_assert_streq("unpinned: verified against the sidecar" "SHA256=${_sidecar_hash}" "${_verified_hash}")
qgc_test_pass("unpinned package comes from upstream, verified against the sidecar")
