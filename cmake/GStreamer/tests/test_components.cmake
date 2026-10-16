cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Components)

if(TEST_CASE STREQUAL "unknown")
    gstreamer_select_components(_selected App Bogus)
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

# component|api|pkg-config module
foreach(_case IN ITEMS "Base|api_base|gstreamer-base-1.0" "GlPrototypes|api_gl_prototypes|gstreamer-gl-prototypes-1.0"
                       "RtspServer|api_rtsp_server|gstreamer-rtsp-server-1.0"
)
    qgc_test_split("${_case}" _component)
    gstreamer_component_names(${_component} _api _pc)
    qgc_test_assert_streq("names of ${_component}" "${_case}" "${_component}|${_api}|${_pc}")
endforeach()
qgc_test_pass("component names")

# The mandatory components come first and every selection keeps GSTREAMER_COMPONENTS order.
gstreamer_select_components(_selected)
qgc_test_assert_streq("mandatory only" "Base;Rtsp;Video" "${_selected}")
gstreamer_select_components(_selected Allocators App GlPrototypes Gl App)
qgc_test_assert_streq("registry order" "Base;Gl;GlPrototypes;Rtsp;Video;App;Allocators" "${_selected}")
qgc_test_pass("select_components")

set(WIN32 OFF)
set(APPLE OFF)
gstreamer_platform_plugin_attrs(_ext _prefix)
qgc_test_assert_streq("linux ext" "so" "${_ext}")
qgc_test_assert_streq("linux prefix" "libgst" "${_prefix}")
qgc_test_pass("platform_plugin_attrs linux")

set(WIN32 ON)
gstreamer_platform_plugin_attrs(_ext _prefix)
qgc_test_assert_streq("win ext" "dll" "${_ext}")
qgc_test_assert_streq("win prefix" "gst" "${_prefix}")
set(WIN32 OFF)
qgc_test_pass("platform_plugin_attrs windows")

set(APPLE ON)
gstreamer_platform_plugin_attrs(_ext _prefix)
qgc_test_assert_streq("apple ext" "dylib" "${_ext}")
qgc_test_assert_streq("apple prefix" "libgst" "${_prefix}")
set(APPLE OFF)
qgc_test_pass("platform_plugin_attrs apple")
