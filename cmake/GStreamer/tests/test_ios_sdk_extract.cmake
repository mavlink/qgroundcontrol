cmake_minimum_required(VERSION 3.25)
# Runs the iOS SDK acquisition in script mode against a pinned stand-in for the 1.28.8+ xcframework tarball: only the
# xcframework's Info.plist and iOS slices are extracted, and each slice resolves to its merged archive and headers.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Acquire)
include("${CMAKE_CURRENT_LIST_DIR}/../platform/IOS.cmake")

qgc_test_sandbox(_sandbox ios-sdk-extract)
set(CPM_SOURCE_CACHE "${_sandbox}/cache")
set(QGC_GST_REQUESTED_VERSION 9.9.2)
set(_cache_dir "${CPM_SOURCE_CACHE}/gstreamer-ios-${QGC_GST_REQUESTED_VERSION}")
set(_archive "${_cache_dir}/gstreamer-ios.tar.xz")

set(_xcfw "${_sandbox}/src/GStreamer.xcframework")
file(WRITE "${_sandbox}/src/.gitignore" "")
file(WRITE "${_xcfw}/Info.plist" "")
foreach(_slice IN ITEMS ios-arm64 ios-arm64_x86_64-simulator tvos-arm64 tvos-arm64_x86_64-simulator)
    file(WRITE "${_xcfw}/${_slice}/libGStreamer.a" "${_slice}")
    file(WRITE "${_xcfw}/${_slice}/Headers/gst/gst.h" "")
endforeach()
file(MAKE_DIRECTORY "${_cache_dir}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cJf "${_archive}" .gitignore GStreamer.xcframework
    WORKING_DIRECTORY "${_sandbox}/src" COMMAND_ERROR_IS_FATAL ANY
)
file(SHA256 "${_archive}" _hash)
set(QGC_BUILD_CONFIG_CONTENT
    "{\"gstreamer\":{\"checksums\":{\"${QGC_GST_REQUESTED_VERSION}\":{\"ios\":\"${_hash}\"}}}}"
)

_qgc_gst_acquire_ios_sdk(_expanded)
qgc_test_assert_streq("expanded dir" "${_cache_dir}/sdk/expanded" "${_expanded}")
file(
    GLOB_RECURSE _extracted
    RELATIVE "${_expanded}"
    "${_expanded}/*"
)
list(SORT _extracted)
set(_expected_files "GStreamer.xcframework/Info.plist")
foreach(_slice IN ITEMS ios-arm64 ios-arm64_x86_64-simulator)
    list(APPEND _expected_files "GStreamer.xcframework/${_slice}/Headers/gst/gst.h"
         "GStreamer.xcframework/${_slice}/libGStreamer.a"
    )
endforeach()
qgc_test_assert_streq("extracted files" "${_expected_files}" "${_extracted}")
qgc_test_pass("iOS xcframework tarball extraction skips tvOS")

foreach(_case IN ITEMS "iphoneos|ios-arm64" "iphonesimulator|ios-arm64_x86_64-simulator")
    qgc_test_split("${_case}" CMAKE_OSX_SYSROOT _slice)
    _qgc_gst_ios_layout("${_expanded}/GStreamer.xcframework" _gst_ios)
    set(_root "${_expanded}/GStreamer.xcframework/${_slice}")
    qgc_test_assert_streq("${CMAKE_OSX_SYSROOT} root" "${_root}" "${_gst_ios_ROOT}")
    qgc_test_assert_streq("${CMAKE_OSX_SYSROOT} lib" "${_root}/libGStreamer.a" "${_gst_ios_LIB}")
    qgc_test_assert_streq("${CMAKE_OSX_SYSROOT} include" "${_root}/Headers" "${_gst_ios_INCLUDE}")
endforeach()
qgc_test_pass("iOS xcframework slices resolve to their merged archive")
