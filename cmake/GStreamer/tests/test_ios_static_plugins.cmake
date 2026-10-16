cmake_minimum_required(VERSION 3.25)
# iOS registers only the requested plugins found in the merged archive: the archive's other plugins (e.g. the GPL
# x264) must not be registered, because registration is what links a plugin in.
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/IOS.cmake")

set(_descriptors _gst_plugin_coreelements_get_desc _gst_plugin_x264_get_desc _gst_plugin_rtp_get_desc
                 _gst_plugin_videoconvertscale_get_desc _gst_plugin_rtpmanager_get_desc
)

# requested|expected registered, lists comma-separated.
set(_cases "rtp,coreelements,vpx,videoconvert,videoconvertscale|rtp,coreelements,videoconvertscale"
           "rtpmanager|rtpmanager" "|"
)
foreach(_case IN LISTS _cases)
    qgc_test_split("${_case}" _requested _expected_registered)
    foreach(_var IN ITEMS _requested _expected_registered)
        string(REPLACE "," ";" ${_var} "${${_var}}")
        list(REMOVE_ITEM ${_var} "")
    endforeach()
    _qgc_gst_ios_select_static_plugins("${_descriptors}" "${_requested}" _registered)
    qgc_test_assert_streq("registered for ${_case}" "${_expected_registered}" "${_registered}")
endforeach()
qgc_test_pass("iOS registers only the requested plugins in the archive")
