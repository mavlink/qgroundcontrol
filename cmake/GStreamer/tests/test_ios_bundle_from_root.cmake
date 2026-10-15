cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/IOS.cmake")

set(_sandbox "${CMAKE_CURRENT_BINARY_DIR}/ios-bundle-from-root")
file(REMOVE_RECURSE "${_sandbox}")
set(_xcfw "${_sandbox}/xc/GStreamer.xcframework")
set(_fw "${_sandbox}/fw/GStreamer.framework")
file(MAKE_DIRECTORY "${_xcfw}/ios-arm64" "${_fw}/Versions/1.0" "${_sandbox}/empty")

# root|expected bundle|expected kind
set(_cases
    "${_xcfw}|${_xcfw}|xcframework"
    "${_xcfw}/|${_xcfw}|xcframework"
    "${_sandbox}/xc|${_xcfw}|xcframework"
    "${_xcfw}/ios-arm64|${_xcfw}|xcframework"
    "${_fw}|${_fw}|framework"
    "${_sandbox}/fw|${_fw}|framework"
    "${_fw}/Versions/1.0|${_fw}|framework"
    "${_sandbox}/empty||"
    "${_sandbox}/missing.xcframework||"
)
foreach(_case IN LISTS _cases)
    string(REPLACE "|" ";" _parts "${_case}")
    list(GET _parts 0 _root)
    list(GET _parts 1 _expected_bundle)
    list(GET _parts 2 _expected_kind)
    _qgc_ios_bundle_from_root("${_root}" _bundle _kind)
    qgc_test_assert_streq("bundle for ${_root}" "${_expected_bundle}" "${_bundle}")
    qgc_test_assert_streq("kind for ${_root}" "${_expected_kind}" "${_kind}")
endforeach()

file(REMOVE_RECURSE "${_sandbox}")
qgc_test_pass("iOS GStreamer_ROOT_DIR to SDK bundle")
