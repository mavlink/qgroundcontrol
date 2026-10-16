cmake_minimum_required(VERSION 3.25)
# An xcframework GStreamer iOS SDK has a static libGStreamer.a per slice and no GStreamer.framework; discovery takes its
# slice from qgc_find_ios_xcframework_slice like the Qt FFmpeg embedding does.
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/IOS.cmake")

qgc_test_sandbox(_sandbox "ios-xcframework-slice${TEST_CASE}")
set(_xcfw "${_sandbox}/GStreamer.xcframework")
foreach(_slice IN ITEMS ios-arm64 ios-arm64_x86_64-simulator ios-arm64_x86_64-maccatalyst)
    file(WRITE "${_xcfw}/${_slice}/libGStreamer.a" "")
endforeach()

if(TEST_CASE STREQUAL "no-matching-arch")
    qgc_find_ios_xcframework_slice(
        XCFRAMEWORK
        "${_xcfw}"
        PLATFORM
        iphoneos
        ARCHITECTURES
        x86_64
        OUT_VAR
        _slice
    )
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

# platform|architectures (comma-separated)|expected slice
set(_cases "iphoneos|arm64|ios-arm64" "iphoneos||ios-arm64" "iphonesimulator|arm64|ios-arm64_x86_64-simulator"
           "iphonesimulator|arm64,x86_64|ios-arm64_x86_64-simulator"
)
foreach(_case IN LISTS _cases)
    qgc_test_split("${_case}" _platform _architectures _expected)
    set(_args XCFRAMEWORK "${_xcfw}" PLATFORM ${_platform} OUT_VAR _slice)
    if(_architectures)
        string(REPLACE "," ";" _architectures "${_architectures}")
        list(APPEND _args ARCHITECTURES ${_architectures})
    endif()
    qgc_find_ios_xcframework_slice(${_args})
    qgc_test_assert_streq("${_case}" "${_xcfw}/${_expected}" "${_slice}")
endforeach()

qgc_test_pass("iOS xcframework slice selection")
