cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Components)

set(_sandbox "${CMAKE_BINARY_DIR}/qgc_plugin_scan_sandbox")
file(REMOVE_RECURSE "${_sandbox}")
file(MAKE_DIRECTORY "${_sandbox}")

gstreamer_scan_plugin_basenames(_empty "${_sandbox}/does-not-exist")
qgc_test_assert_streq("missing path returns empty" "" "${_empty}")
qgc_test_pass("missing path")

gstreamer_platform_plugin_attrs(_ext _prefix _glob)
file(TOUCH "${_sandbox}/${_prefix}coreelements.${_ext}")
file(TOUCH "${_sandbox}/${_prefix}playback.${_ext}")
file(TOUCH "${_sandbox}/${_prefix}videoconvertscale.${_ext}")
file(TOUCH "${_sandbox}/${_prefix}x264.${_ext}")
file(TOUCH "${_sandbox}/notaplugin.${_ext}")            # Non-plugin file — must be excluded by the plugin glob
file(TOUCH "${_sandbox}/${_prefix}-helper.${_ext}")     # Dash-prefixed name — alphanumeric regex captures empty group, deduped away

gstreamer_scan_plugin_basenames(_names "${_sandbox}")

qgc_test_assert_in_list("found coreelements"        coreelements        _names)
qgc_test_assert_in_list("found playback"            playback            _names)
qgc_test_assert_in_list("found videoconvertscale"   videoconvertscale   _names)
qgc_test_assert_in_list("found x264"                x264                _names)
qgc_test_assert_not_in_list("notaplugin not scanned" notaplugin _names)
qgc_test_pass("scan_plugin_basenames platform")

file(REMOVE_RECURSE "${_sandbox}")
