cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Json)
include(PluginPolicy)

set(QGC_BUILD_CONFIG_CONTENT [[{
    "gstreamer": {
        "plugins": {
            "common": ["coreelements", "playback", "rtsp"],
            "linux":  ["va"],
            "apple":  ["vtenc", "vtdec"]
        }
    }
}]])

gstreamer_plugins_for(PLATFORM "" OUT_VAR _plugins_common)
qgc_test_assert_in_list("common has coreelements" coreelements _plugins_common)
qgc_test_assert_in_list("common has playback"     playback     _plugins_common)
qgc_test_assert_not_in_list("common excludes va"  va           _plugins_common)
qgc_test_pass("plugins_for common-only")

gstreamer_plugins_for(PLATFORM linux OUT_VAR _plugins_linux)
qgc_test_assert_in_list("linux has coreelements" coreelements _plugins_linux)
qgc_test_assert_in_list("linux has va"           va           _plugins_linux)
qgc_test_assert_not_in_list("linux excludes vtenc" vtenc      _plugins_linux)
qgc_test_pass("plugins_for linux addenda")

gstreamer_plugins_for(PLATFORM apple OUT_VAR _plugins_apple)
qgc_test_assert_in_list("apple has vtenc" vtenc _plugins_apple)
qgc_test_assert_in_list("apple has vtdec" vtdec _plugins_apple)
qgc_test_pass("plugins_for apple addenda")

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../.github/build-config.json" QGC_BUILD_CONFIG_CONTENT)
gstreamer_plugins_for(PLATFORM windows OUT_VAR _plugins_windows_real)
qgc_test_assert_in_list("windows has d3d11" d3d11 _plugins_windows_real)
qgc_test_assert_in_list("windows has d3d12" d3d12 _plugins_windows_real)
qgc_test_pass("plugins_for real windows d3d addenda")

set(_req videoconvertscale videoconvert videoscale x264enc)
set(_avail videoconvertscale x264enc)
gstreamer_filter_alternates(IN_OUT_PLUGINS _req AVAILABLE ${_avail})
qgc_test_assert_in_list("filtered: videoconvertscale retained (present)" videoconvertscale _req)
qgc_test_assert_not_in_list("filtered: videoconvert removed (absent)"    videoconvert      _req)
qgc_test_assert_not_in_list("filtered: videoscale removed (absent)"      videoscale        _req)
qgc_test_assert_in_list("filtered: x264enc retained"                     x264enc           _req)
qgc_test_pass("filter_alternates fused-satisfied")

set(_req2 videoconvertscale videoconvert videoscale)
set(_avail2 videoconvert videoscale)
gstreamer_filter_alternates(IN_OUT_PLUGINS _req2 AVAILABLE ${_avail2})
qgc_test_assert_not_in_list("split-sat: videoconvertscale removed (absent)" videoconvertscale _req2)
qgc_test_assert_in_list("split-sat: videoconvert retained (present)"        videoconvert      _req2)
qgc_test_assert_in_list("split-sat: videoscale retained (present)"          videoscale        _req2)
qgc_test_pass("filter_alternates split-pair satisfied")

set(_req3 videoconvertscale videoconvert videoscale x264enc)
set(_avail3 x264enc)
gstreamer_filter_alternates(IN_OUT_PLUGINS _req3 AVAILABLE ${_avail3})
qgc_test_assert_in_list("unsat: videoconvertscale retained" videoconvertscale _req3)
qgc_test_assert_in_list("unsat: videoconvert retained"      videoconvert      _req3)
qgc_test_assert_in_list("unsat: videoscale retained"        videoscale        _req3)
qgc_test_pass("filter_alternates none-satisfied")

set(_req_partial videoconvertscale videoconvert videoscale x264enc)
set(_avail_partial videoconvert x264enc)
gstreamer_filter_alternates(IN_OUT_PLUGINS _req_partial AVAILABLE ${_avail_partial})
qgc_test_assert_in_list("partial: videoconvertscale retained" videoconvertscale _req_partial)
qgc_test_assert_in_list("partial: videoconvert retained"      videoconvert      _req_partial)
qgc_test_assert_in_list("partial: videoscale retained (absent half)" videoscale _req_partial)
qgc_test_assert_in_list("partial: x264enc retained"           x264enc           _req_partial)
qgc_test_pass("filter_alternates partial-pair unsatisfied")

gstreamer_runtime_required_plugins(_required)
foreach(_p IN ITEMS coreelements isomp4 matroska multifile opengl playback rtsp rtp rtpmanager tcp udp videoconvertscale)
    qgc_test_assert_in_list("runtime required: ${_p}" "${_p}" _required)
endforeach()
qgc_test_assert_not_in_list("runtime required: openh264 is optional codec implementation" openh264 _required)
qgc_test_pass("runtime_required_plugins")

gstreamer_plugin_satisfy_sets(PLUGIN videoconvertscale OUT_VAR _sets_fused)
qgc_test_assert_in_list("satisfy: fused alternative present" videoconvertscale _sets_fused)
qgc_test_assert_in_list("satisfy: split alternative present" "videoconvert+videoscale" _sets_fused)
qgc_test_pass("plugin_satisfy_sets grouped")

gstreamer_plugin_satisfy_sets(PLUGIN coreelements OUT_VAR _sets_plain)
qgc_test_assert_in_list("satisfy: plain returns self" coreelements _sets_plain)
list(LENGTH _sets_plain _plain_len)
qgc_test_assert_streq("satisfy: plain has one set" 1 "${_plain_len}")
qgc_test_pass("plugin_satisfy_sets ungrouped")

gstreamer_xcfw_skip(_skip)
qgc_test_assert_in_list("xcfw skip: x265" x265 _skip)
qgc_test_pass("xcfw_skip")
