# Test: gstreamer_scan_plugin_basenames extracts plugin basenames from a
# directory of mock plugin files (only the names matter, not the contents).

cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Components)

set(_sandbox "${CMAKE_BINARY_DIR}/qgc_plugin_scan_sandbox")
file(REMOVE_RECURSE "${_sandbox}")
file(MAKE_DIRECTORY "${_sandbox}")

# ── empty path ──────────────────────────────────────────────────────────────
gstreamer_scan_plugin_basenames(_empty "${_sandbox}/does-not-exist")
qgc_test_assert_streq("missing path returns empty" "" "${_empty}")
qgc_test_pass("missing path")

# ── linux-style plugin set (libgst*.so) ─────────────────────────────────────
# Tests run on Linux so platform_plugin_attrs returns libgst/.so.
file(TOUCH "${_sandbox}/libgstcoreelements.so")
file(TOUCH "${_sandbox}/libgstplayback.so")
file(TOUCH "${_sandbox}/libgstvideoconvertscale.so")
file(TOUCH "${_sandbox}/libgstx264.so")
file(TOUCH "${_sandbox}/notaplugin.so")            # Non-plugin file — must be excluded by the libgst* glob
file(TOUCH "${_sandbox}/libgst-helper.so")         # Dash-prefixed name — alphanumeric regex captures empty group, deduped away

gstreamer_scan_plugin_basenames(_names "${_sandbox}")

qgc_test_assert_in_list("found coreelements"        coreelements        _names)
qgc_test_assert_in_list("found playback"            playback            _names)
qgc_test_assert_in_list("found videoconvertscale"   videoconvertscale   _names)
qgc_test_assert_in_list("found x264"                x264                _names)
qgc_test_assert_not_in_list("notaplugin not scanned" notaplugin _names)
qgc_test_pass("scan_plugin_basenames linux")

file(REMOVE_RECURSE "${_sandbox}")
