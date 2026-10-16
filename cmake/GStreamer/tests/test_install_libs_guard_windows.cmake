cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Install)

# Script mode reports the host platform; the guard keys off the target's WIN32.
set(WIN32 TRUE)

_qgc_gst_is_blocked_lib_copy_source("C:/Program Files/gstreamer/1.0/msvc_x86_64/bin" _blocked_gst)
qgc_test_assert_streq("allows Program Files GStreamer SDK" FALSE "${_blocked_gst}")

_qgc_gst_is_blocked_lib_copy_source("C:/Program Files/Common Files/vendor/bin" _blocked_common)
qgc_test_assert_streq("blocks unrelated Program Files source" TRUE "${_blocked_common}")

_qgc_gst_is_blocked_lib_copy_source("C:/Windows/System32" _blocked_windows)
qgc_test_assert_streq("blocks Windows system source" TRUE "${_blocked_windows}")

qgc_test_pass("install_libs_guard_windows")
