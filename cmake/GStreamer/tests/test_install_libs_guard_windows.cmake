cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Install)

set(QGC_GSTREAMER_TEST_WIN32 TRUE)

_gstreamer_is_blocked_lib_copy_source("C:/Program Files/gstreamer/1.0/msvc_x86_64/bin" _blocked_gst)
qgc_test_assert_streq("allows Program Files GStreamer SDK" FALSE "${_blocked_gst}")

_gstreamer_is_blocked_lib_copy_source("C:/Program Files/Common Files/vendor/bin" _blocked_common)
qgc_test_assert_streq("blocks unrelated Program Files source" TRUE "${_blocked_common}")

_gstreamer_is_blocked_lib_copy_source("C:/Windows/System32" _blocked_windows)
qgc_test_assert_streq("blocks Windows system source" TRUE "${_blocked_windows}")

qgc_test_pass("install_libs_guard_windows")
