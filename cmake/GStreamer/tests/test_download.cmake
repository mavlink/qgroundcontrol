# Test: gstreamer_get_package_url, gstreamer_get_s3_mirror_url,
# gstreamer_get_recommended_version. (qgc_parse_expected_hash now lives in
# cmake/modules/Download.cmake and is covered by test_download_helper.cmake.)
# Network-touching helpers (resilient_download / fetch_checksum) are NOT
# exercised here — that belongs in an integration test gated by a Network
# label.

cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_LIST_DIR}/.."
    "${CMAKE_CURRENT_LIST_DIR}/../../modules"
)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Download)

# ── package_url for known platforms ─────────────────────────────────────────
gstreamer_get_package_url(android  1.28.2 _u_android)
qgc_test_assert_streq("android url" "https://gstreamer.freedesktop.org/data/pkg/android/1.28.2/gstreamer-1.0-android-universal-1.28.2.tar.xz" "${_u_android}")

gstreamer_get_package_url(ios      1.28.2 _u_ios)
qgc_test_assert_streq("ios url" "https://gstreamer.freedesktop.org/data/pkg/ios/1.28.2/gstreamer-1.0-devel-1.28.2-ios-universal.pkg" "${_u_ios}")

gstreamer_get_package_url(macos    1.28.2 _u_mac)
qgc_test_assert_streq("macos url" "https://gstreamer.freedesktop.org/data/pkg/macos/1.28.2/gstreamer-1.0-1.28.2-universal.pkg" "${_u_mac}")

gstreamer_get_package_url(macos_devel 1.28.2 _u_macd)
qgc_test_assert_streq("macos_devel url" "https://gstreamer.freedesktop.org/data/pkg/macos/1.28.2/gstreamer-1.0-devel-1.28.2-universal.pkg" "${_u_macd}")

gstreamer_get_package_url(windows_msvc_x64   1.28.2 _u_winx)
qgc_test_assert_streq("windows x64 url" "https://gstreamer.freedesktop.org/data/pkg/windows/1.28.2/msvc/gstreamer-1.0-msvc-x86_64-1.28.2.exe" "${_u_winx}")

gstreamer_get_package_url(windows_msvc_arm64 1.28.2 _u_wina)
qgc_test_assert_streq("windows arm64 url" "https://gstreamer.freedesktop.org/data/pkg/windows/1.28.2/msvc/gstreamer-1.0-msvc-arm64-1.28.2.exe" "${_u_wina}")

qgc_test_pass("get_package_url known platforms")

# ── s3 mirror routing ───────────────────────────────────────────────────────
gstreamer_get_s3_mirror_url(android 1.28.2 _s3_android)
qgc_test_assert_streq("s3 android" "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/android/gstreamer-1.0-android-universal-1.28.2.tar.xz" "${_s3_android}")

gstreamer_get_s3_mirror_url(macos 1.28.2 _s3_mac)
qgc_test_assert_streq("s3 macos" "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/macos/gstreamer-1.0-1.28.2-universal.pkg" "${_s3_mac}")

gstreamer_get_s3_mirror_url(windows_msvc_x64 1.28.2 _s3_win)
qgc_test_assert_streq("s3 windows" "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer/windows/gstreamer-1.0-msvc-x86_64-1.28.2.exe" "${_s3_win}")

# Unsupported platform returns empty
gstreamer_get_s3_mirror_url(linux 1.28.2 _s3_linux)
qgc_test_assert_streq("s3 linux empty (no mirror)" "" "${_s3_linux}")

qgc_test_pass("get_s3_mirror_url routing")

# ── recommended_version: detected patch from input ──────────────────────────
gstreamer_get_recommended_version(1.28.2 _v_detected)
qgc_test_assert_streq("detected patch from input" "1.28.2" "${_v_detected}")

# ── recommended_version: explicit override ──────────────────────────────────
set(QGC_GSTREAMER_PATCH_1_28 5)
gstreamer_get_recommended_version(1.28.0 _v_override)
qgc_test_assert_streq("explicit override" "1.28.5" "${_v_override}")
unset(QGC_GSTREAMER_PATCH_1_28)
qgc_test_pass("recommended_version")

