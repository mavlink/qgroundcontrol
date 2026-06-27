cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Install)

set(GSTREAMER_PLUGINS video coreelements)

set(_paths
    /sdk/libgstvideo.so
    /sdk/libgstvideoconvert.so
    /sdk/libgstcoreelements.so
    /sdk/libgstbadplugin.so
)
_gstreamer_filter_plugin_paths("libgst" "${_paths}" _kept)

qgc_test_assert_in_list("keeps exact video"        "/sdk/libgstvideo.so"        _kept)
qgc_test_assert_in_list("keeps coreelements"       "/sdk/libgstcoreelements.so" _kept)
list(FIND _kept "/sdk/libgstvideoconvert.so" _vc_idx)
qgc_test_assert_streq("rejects videoconvert (boundary)" "-1" "${_vc_idx}")
list(FIND _kept "/sdk/libgstbadplugin.so" _bad_idx)
qgc_test_assert_streq("rejects non-allowlisted" "-1" "${_bad_idx}")
qgc_test_pass("filter_plugin_paths boundary")
