cmake_minimum_required(VERSION 3.25)
# The iOS SDK version comes from gst/gstversion.h, which every SDK layout has, including an xcframework slice that holds
# only libGStreamer.a and Headers (no GStreamer.framework/Info.plist, which the version used to be read from).
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../SdkState.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/IOS.cmake")

qgc_test_sandbox(_sandbox "ios-sdk-version${TEST_CASE}")
set(QGC_GST_REQUESTED_VERSION 1.28.4)
set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_OSX_ARCHITECTURES arm64)

# Writes <dir>/gst/gstversion.h for <major>.<minor>.<micro> as GStreamer generates it.
function(_qgc_test_version_header dir version)
    string(REPLACE "." ";" _parts "${version}")
    list(GET _parts 0 _major)
    list(GET _parts 1 _minor)
    list(GET _parts 2 _micro)
    file(WRITE "${dir}/gst/gstversion.h"
         "/**\n * GST_VERSION_MAJOR:\n */\n#define GST_VERSION_MAJOR (${_major})\n"
         "#define GST_VERSION_MINOR (${_minor})\n#define GST_VERSION_MICRO (${_micro})\n#define GST_VERSION_NANO (0)\n"
    )
endfunction()

# Writes a GStreamer.framework under <dir> with a merged binary and headers for <version>.
function(_qgc_test_framework dir version)
    file(WRITE "${dir}/GStreamer.framework/Versions/1.0/GStreamer" "")
    _qgc_test_version_header("${dir}/GStreamer.framework/Versions/1.0/Headers" ${version})
endfunction()

if(TEST_CASE STREQUAL "below-floor")
    _qgc_test_framework("${_sandbox}" 1.26.10)
    _qgc_gst_ios_layout("${_sandbox}/GStreamer.framework" _layout)
    _qgc_gst_ios_sdk_version("${_layout_INCLUDE}" _version)
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

set(_xcfw "${_sandbox}/GStreamer.xcframework")
foreach(_slice IN ITEMS ios-arm64 ios-arm64_x86_64-simulator)
    file(WRITE "${_xcfw}/${_slice}/libGStreamer.a" "")
    _qgc_test_version_header("${_xcfw}/${_slice}/Headers" 1.28.9)
endforeach()
_qgc_gst_ios_layout("${_xcfw}" _layout)
qgc_test_assert_streq("slice archive" "${_xcfw}/ios-arm64/libGStreamer.a" "${_layout_LIB}")
_qgc_gst_ios_sdk_version("${_layout_INCLUDE}" _version)
qgc_test_assert_streq("xcframework slice version" 1.28.9 "${_version}")
qgc_test_pass("xcframework slice without Info.plist")

_qgc_test_framework("${_sandbox}/fw" 1.28.7)
_qgc_gst_ios_layout("${_sandbox}/fw/GStreamer.framework" _layout)
qgc_test_assert_streq("framework archive" "${_sandbox}/fw/GStreamer.framework/Versions/1.0/GStreamer" "${_layout_LIB}")
_qgc_gst_ios_sdk_version("${_layout_INCLUDE}" _version)
qgc_test_assert_streq("framework version" 1.28.7 "${_version}")
qgc_test_pass("framework")

file(MAKE_DIRECTORY "${_sandbox}/no-headers")
_qgc_gst_ios_sdk_version("${_sandbox}/no-headers" _version)
qgc_test_assert_streq("unreadable version falls back to the requested one" 1.28.4 "${_version}")
qgc_test_pass("no version header")
