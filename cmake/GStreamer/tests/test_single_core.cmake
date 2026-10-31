cmake_minimum_required(VERSION 3.25)
# The Android and iOS apps must take the GStreamer core from GStreamer::mobile. The post-build check
# (SingleCoreCheck.cmake) flags an app binary that defines core symbols itself, which means it linked a second core.
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../StaticInit.cmake")

# symbol prefix|platform: Mach-O symbols carry a leading underscore, ELF ones do not.
foreach(_case IN ITEMS "_|iOS" "|Android")
    qgc_test_split("${_case}" _p _platform)
    # Defined in GStreamer::mobile only: the app's own symbols and imports.
    set(_single_core "0000000100001000 T ${_p}main\n0000000100002000 T ${_p}gst_element_register_qgcvideosinkbin\n")
    qgc_gst_app_core_symbols("${_single_core}" "${_p}" _found)
    qgc_test_assert_streq("${_platform} single core" "" "${_found}")

    # A second core linked into the app, with hidden visibility as a local (t) symbol.
    set(_two_cores "0000000100003000 t ${_p}gst_element_get_type\n0000000100004000 T ${_p}gst_init_check")
    qgc_gst_app_core_symbols("${_single_core}${_two_cores}" "${_p}" _found)
    qgc_test_assert_streq("${_platform} second core" "${_p}gst_init_check;${_p}gst_element_get_type" "${_found}")

    # A QGC symbol that only contains a core name is not a core symbol.
    qgc_gst_app_core_symbols("0000000100005000 T ${_p}gst_init_check_qgc\n" "${_p}" _found)
    qgc_test_assert_streq("${_platform} prefix only" "" "${_found}")
endforeach()

list(LENGTH QGC_GST_APP_CORE_SYMBOLS _forced)
if(_forced EQUAL 0)
    message(FATAL_ERROR "ASSERT: QGC_GST_APP_CORE_SYMBOLS must list the core functions QGC calls")
endif()
qgc_test_pass("mobile app core check detects a second GStreamer core")
