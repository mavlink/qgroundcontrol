cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/IOS.cmake")

qgc_test_sandbox(_sandbox ios-bundle-from-root)
set(_xcfw "${_sandbox}/xc/GStreamer.xcframework")
set(_fw "${_sandbox}/fw/GStreamer.framework")
file(MAKE_DIRECTORY "${_xcfw}/ios-arm64" "${_fw}/Versions/1.0" "${_sandbox}/empty")

# root|expected bundle
set(_cases
    "${_xcfw}|${_xcfw}"
    "${_xcfw}/|${_xcfw}"
    "${_sandbox}/xc|${_xcfw}"
    "${_xcfw}/ios-arm64|${_xcfw}"
    "${_fw}|${_fw}"
    "${_sandbox}/fw|${_fw}"
    "${_fw}/Versions/1.0|${_fw}"
    "${_sandbox}/empty|"
    "${_sandbox}/missing.xcframework|"
)
foreach(_case IN LISTS _cases)
    qgc_test_split("${_case}" _root _expected_bundle)
    _qgc_gst_ios_bundle_from_root("${_root}" _bundle)
    qgc_test_assert_streq("bundle for ${_root}" "${_expected_bundle}" "${_bundle}")
endforeach()

qgc_test_pass("iOS GStreamer_ROOT_DIR to SDK bundle")
