cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Components)

if(TEST_CASE STREQUAL "unknown")
    qgc_gst_select_components(_selected App Bogus)
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

# component|pkg-config module
foreach(
    _case IN ITEMS "Base|gstreamer-base-1.0" "GlPrototypes|gstreamer-gl-prototypes-1.0"
                   "RtspServer|gstreamer-rtsp-server-1.0"
)
    qgc_test_split("${_case}" _component)
    qgc_gst_component_module(${_component} _module)
    qgc_test_assert_streq("module of ${_component}" "${_case}" "${_component}|${_module}")
endforeach()
qgc_test_pass("component modules")

# The mandatory components come first and every selection keeps GSTREAMER_COMPONENTS order.
qgc_gst_select_components(_selected)
qgc_test_assert_streq("mandatory only" "Base;Rtsp;Video" "${_selected}")
qgc_gst_select_components(_selected Allocators App GlPrototypes Gl App)
qgc_test_assert_streq("registry order" "Base;Gl;GlPrototypes;Rtsp;Video;App;Allocators" "${_selected}")
qgc_test_pass("select_components")
