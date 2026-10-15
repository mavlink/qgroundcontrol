cmake_minimum_required(VERSION 3.25)
# The iOS app must take the GStreamer core from GStreamer::mobile. The post-build check flags an app binary that defines
# core symbols itself, which means it linked a second core from the merged archive.
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/IOS.cmake")

# Defined in GStreamer::mobile only: the app's own symbols and imports.
set(_single_core "0000000100001000 T _main\n0000000100002000 T _gst_element_register_qgcvideosinkbin\n")
_qgc_gst_ios_app_core_symbols("${_single_core}" _found)
qgc_test_assert_streq("single core" "" "${_found}")

# A second core linked into the app.
string(APPEND _two_cores "0000000100003000 T _gst_element_get_type\n0000000100004000 T _gst_init_check")
_qgc_gst_ios_app_core_symbols("${_single_core}${_two_cores}" _found)
qgc_test_assert_streq("second core" "_gst_init_check;_gst_element_get_type" "${_found}")

# A QGC symbol that only contains a core name is not a core symbol.
_qgc_gst_ios_app_core_symbols("0000000100005000 T _gst_init_check_qgc\n" _found)
qgc_test_assert_streq("prefix only" "" "${_found}")

list(LENGTH QGC_GST_IOS_APP_CORE_SYMBOLS _forced)
if(_forced EQUAL 0)
    message(FATAL_ERROR "ASSERT: QGC_GST_IOS_APP_CORE_SYMBOLS must list the core functions QGC calls")
endif()
qgc_test_pass("iOS app core check detects a second GStreamer core")
