cmake_minimum_required(VERSION 3.22)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Windows.cmake")

set(_sdk_root "${CMAKE_CURRENT_BINARY_DIR}/windows-sdk-version")
file(REMOVE_RECURSE "${_sdk_root}")
file(MAKE_DIRECTORY "${_sdk_root}/lib/pkgconfig")

file(WRITE "${_sdk_root}/lib/pkgconfig/gstreamer-1.0.pc" "Version: 1.22.12\n")
_qgc_windows_sdk_version("${_sdk_root}" _version)
qgc_test_assert_streq("SDK version" "1.22.12" "${_version}")

file(WRITE "${_sdk_root}/lib/pkgconfig/gstreamer-1.0.pc" "prefix=/invalid\n")
_qgc_windows_sdk_version("${_sdk_root}" _version)
qgc_test_assert_streq("missing SDK version" "" "${_version}")

file(REMOVE_RECURSE "${_sdk_root}")
qgc_test_pass("Windows SDK version detection")
