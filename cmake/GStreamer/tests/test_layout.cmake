cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Layout)

set(_sandbox "${CMAKE_BINARY_DIR}/qgc_layout_test_sandbox")
file(REMOVE_RECURSE "${_sandbox}")
file(MAKE_DIRECTORY "${_sandbox}")

set(_flat "${_sandbox}/flat-sdk")
file(MAKE_DIRECTORY "${_flat}/lib/gstreamer-1.0")
file(MAKE_DIRECTORY "${_flat}/include")

gstreamer_create_layout_target(SDK_ROOT "${_flat}" TYPE FLAT)

qgc_test_assert_streq("FLAT lib"     "${_flat}/lib"             "${GSTREAMER_LIB_PATH}")
qgc_test_assert_streq("FLAT plugins" "${_flat}/lib/gstreamer-1.0" "${GSTREAMER_PLUGIN_PATH}")
qgc_test_assert_streq("FLAT include" "${_flat}/include"         "${GSTREAMER_INCLUDE_PATH}")
qgc_test_pass("FLAT layout")

set(_flat_multi "${_sandbox}/flat-multi")
file(MAKE_DIRECTORY "${_flat_multi}/lib/x86_64-linux-gnu/gstreamer-1.0")
file(MAKE_DIRECTORY "${_flat_multi}/include")

gstreamer_create_layout_target(
    SDK_ROOT "${_flat_multi}"
    TYPE FLAT
    LIB_PATH "${_flat_multi}/lib/x86_64-linux-gnu"
)
qgc_test_assert_streq("FLAT multiarch lib"     "${_flat_multi}/lib/x86_64-linux-gnu"             "${GSTREAMER_LIB_PATH}")
qgc_test_assert_streq("FLAT multiarch plugins" "${_flat_multi}/lib/x86_64-linux-gnu/gstreamer-1.0" "${GSTREAMER_PLUGIN_PATH}")
qgc_test_pass("FLAT lib_path override")

set(_static "${_sandbox}/static")
file(MAKE_DIRECTORY "${_static}/lib/gstreamer-1.0")
file(MAKE_DIRECTORY "${_static}/include")

gstreamer_create_layout_target(SDK_ROOT "${_static}" TYPE STATIC_TARBALL)
qgc_test_assert_streq("STATIC_TARBALL lib"  "${_static}/lib" "${GSTREAMER_LIB_PATH}")
qgc_test_pass("STATIC_TARBALL layout")

set(_fw_root "${_sandbox}/fw-root")
file(MAKE_DIRECTORY "${_fw_root}/lib/gstreamer-1.0")
file(MAKE_DIRECTORY "${_fw_root}/include")
set(_fw_bundle "${_sandbox}/GStreamer.framework")
file(MAKE_DIRECTORY "${_fw_bundle}")

gstreamer_create_layout_target(
    SDK_ROOT         "${_fw_root}"
    TYPE             FRAMEWORK
    FRAMEWORK_BUNDLE "${_fw_bundle}"
)
qgc_test_assert_streq("FRAMEWORK lib"     "${_fw_root}/lib"             "${GSTREAMER_LIB_PATH}")
qgc_test_assert_streq("FRAMEWORK plugins" "${_fw_root}/lib/gstreamer-1.0" "${GSTREAMER_PLUGIN_PATH}")
qgc_test_assert_streq("FRAMEWORK_PATH"    "${_fw_bundle}"               "${GSTREAMER_FRAMEWORK_PATH}")
qgc_test_pass("FRAMEWORK layout")

set(_xcfw_root "${_sandbox}/xcfw-root")
file(MAKE_DIRECTORY "${_xcfw_root}/Headers")
set(_xcfw_bundle "${_sandbox}/GStreamer.xcframework")
file(MAKE_DIRECTORY "${_xcfw_bundle}")

gstreamer_create_layout_target(
    SDK_ROOT           "${_xcfw_root}"
    TYPE               XCFRAMEWORK
    INCLUDE_PATH       "${_xcfw_root}/Headers"
    XCFRAMEWORK_BUNDLE "${_xcfw_bundle}"
)
qgc_test_assert_streq("XCFRAMEWORK lib (slice)"     "${_xcfw_root}"        "${GSTREAMER_LIB_PATH}")
qgc_test_assert_streq("XCFRAMEWORK plugins (slice)" "${_xcfw_root}"        "${GSTREAMER_PLUGIN_PATH}")
qgc_test_assert_streq("XCFRAMEWORK include"         "${_xcfw_root}/Headers" "${GSTREAMER_INCLUDE_PATH}")
qgc_test_assert_streq("XCFRAMEWORK_PATH"            "${_xcfw_bundle}"      "${GSTREAMER_XCFRAMEWORK_PATH}")
qgc_test_pass("XCFRAMEWORK layout")

gstreamer_layout_get(LIB_DIR _none_lib)
qgc_test_assert_streq("layout_get LIB_DIR (no target)" "" "${_none_lib}")
gstreamer_layout_get(FRAMEWORK_BUNDLE _none_fw)
qgc_test_assert_streq("layout_get FRAMEWORK_BUNDLE (no target)" "" "${_none_fw}")
qgc_test_pass("layout_get script-mode safety")

gstreamer_layout_set(FOO bar)
qgc_test_pass("layout_set script-mode safety")

file(REMOVE_RECURSE "${_sandbox}")
