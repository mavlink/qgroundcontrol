cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(PluginPolicy)

set(QGC_BUILD_CONFIG_CONTENT
    [[{
    "gstreamer": {
        "plugins": {
            "common": ["coreelements", "playback", "rtsp"],
            "linux":  ["va"],
            "apple":  ["vtenc", "vtdec"]
        }
    }
}]]
)

gstreamer_plugins_for("" _plugins_common)
qgc_test_assert_in_list("common has coreelements" coreelements _plugins_common)
qgc_test_assert_in_list("common has playback" playback _plugins_common)
qgc_test_assert_not_in_list("common excludes va" va _plugins_common)
qgc_test_pass("plugins_for common-only")

gstreamer_plugins_for(linux _plugins_linux)
qgc_test_assert_in_list("linux has coreelements" coreelements _plugins_linux)
qgc_test_assert_in_list("linux has va" va _plugins_linux)
qgc_test_assert_not_in_list("linux excludes vtenc" vtenc _plugins_linux)
qgc_test_pass("plugins_for linux addenda")

gstreamer_plugins_for(apple _plugins_apple)
qgc_test_assert_in_list("apple has vtenc" vtenc _plugins_apple)
qgc_test_assert_in_list("apple has vtdec" vtdec _plugins_apple)
qgc_test_pass("plugins_for apple addenda")

# required|available|expected missing, lists comma-separated. videoconvertscale is satisfied by itself or by
# videoconvert+videoscale; each of those two is satisfied by itself or by videoconvertscale.
set(_missing_cases
    "videoconvertscale,videoconvert,videoscale,x264enc|videoconvertscale,x264enc|"
    "videoconvertscale,videoconvert,videoscale|videoconvert,videoscale|"
    "videoconvertscale,videoconvert,videoscale,x264enc|x264enc|videoconvertscale,videoconvert,videoscale"
    "videoconvertscale,videoconvert,videoscale||videoconvertscale,videoconvert,videoscale"
    "videoconvertscale,videoconvert,videoscale,x264enc|videoconvert,x264enc|videoconvertscale,videoscale"
    "coreelements,rtsp|rtsp|coreelements"
    "|coreelements|"
)
foreach(_case IN LISTS _missing_cases)
    qgc_test_split("${_case}" _required _available _expected)
    string(REPLACE "," ";" _required "${_required}")
    string(REPLACE "," ";" _available "${_available}")
    string(REPLACE "," ";" _expected "${_expected}")
    gstreamer_missing_required(
        REQUIRED ${_required}
        AVAILABLE ${_available}
        OUT_VAR _missing
    )
    qgc_test_assert_streq("missing for ${_case}" "${_expected}" "${_missing}")
endforeach()
qgc_test_pass("missing_required honours alternate sets")

# Missing optional decoders are only reported; the other missing plugins are absent from the SDK, or shipped but not
# usable by the build. The fused videoconvertscale is satisfied by the shipped videoconvert+videoscale.
gstreamer_classify_missing_plugins(
    REQUESTED coreelements qsv typo udp videoconvertscale
    AVAILABLE coreelements
    SHIPPED coreelements udp videoconvert videoscale
    OUT_OPTIONAL _optional
    OUT_ABSENT _absent
    OUT_UNUSABLE _unusable
)
qgc_test_assert_streq("optional" qsv "${_optional}")
qgc_test_assert_streq("absent" typo "${_absent}")
qgc_test_assert_streq("unusable" "udp;videoconvertscale" "${_unusable}")
qgc_test_pass("classify_missing_plugins")

# plugin|expected alternatives, "|"-separated
foreach(
    _case IN ITEMS "coreelements|coreelements" "videoconvertscale|videoconvertscale|videoconvert+videoscale"
                   "videoscale|videoscale|videoconvertscale|videoconvert+videoscale"
)
    string(REPLACE "|" ";" _parts "${_case}")
    list(POP_FRONT _parts _plugin)
    gstreamer_plugin_alternatives(${_plugin} _alternatives)
    qgc_test_assert_streq("alternatives of ${_plugin}" "${_parts}" "${_alternatives}")
endforeach()
qgc_test_pass("plugin_alternatives")

# Neither the allowlist nor the mobile Debug plugins may name a GPL plugin.
file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../.github/build-config.json" QGC_BUILD_CONFIG_CONTENT)
foreach(_key IN ITEMS android apple windows linux)
    gstreamer_plugins_for(${_key} _plugins)
    list(TRANSFORM _plugins PREPEND libgst)
    gstreamer_gpl_files(_gpl ${_plugins} ${GSTREAMER_MOCK_SERVER_DEBUG_PLUGINS})
    qgc_test_assert_streq("GPL plugins for ${_key}" "" "${_gpl}")
endforeach()
qgc_test_pass("policy lists")

# GPL libraries and plugins by file name, on every platform; names merely starting like one are not.
set(_gpl_files
    /sdk/lib/libx264.so.164
    /sdk/bin/libx264-164.dll
    /sdk/lib/libx265.215.dylib
    /sdk/lib/gstreamer-1.0/libgstx264.so
    /sdk/lib/gstreamer-1.0/gstx265.dll
    /sdk/lib/libmpeg2.0.dylib
    /sdk/lib/gstreamer-1.0/libgsta52dec.dylib
    /sdk/bin/dvdread-8.dll
)
set(_lgpl_files /sdk/lib/libgstsidebyside.so /sdk/lib/libx2640.so /sdk/lib/gstreamer-1.0/libgstopenh264.so
                /sdk/lib/libavcodec.so.61
)
gstreamer_gpl_files(_found ${_gpl_files} ${_lgpl_files})
qgc_test_assert_streq("GPL files" "${_gpl_files}" "${_found}")
qgc_test_pass("gpl_files")
