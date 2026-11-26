cmake_minimum_required(VERSION 3.25)
# An Android GStreamer_ROOT_DIR is one ABI's SDK tree, or a universal SDK, whose tree for the build's ABI is used.
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Android.cmake")

qgc_test_sandbox(_sandbox "android-abi-root${TEST_CASE}")
set(_pc lib/pkgconfig/gstreamer-1.0.pc)
file(WRITE "${_sandbox}/one-abi/${_pc}" "")
file(WRITE "${_sandbox}/universal/arm64/${_pc}" "")
file(WRITE "${_sandbox}/universal/x86_64/${_pc}" "")

if(TEST_CASE STREQUAL "no-tree")
    # The universal SDK here has no armv7 tree.
    _qgc_gst_android_abi_root("${_sandbox}/universal" armeabi-v7a _root)
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

# root|ABI|expected tree, paths under the sandbox
set(_cases "one-abi|arm64-v8a|one-abi" "one-abi/|x86_64|one-abi" "universal|arm64-v8a|universal/arm64"
           "universal/|x86_64|universal/x86_64" "universal/x86_64|x86_64|universal/x86_64"
)
foreach(_case IN LISTS _cases)
    qgc_test_split("${_case}" _root _abi _expected)
    _qgc_gst_android_abi_root("${_sandbox}/${_root}" ${_abi} _tree)
    qgc_test_assert_streq("${_case}" "${_sandbox}/${_expected}" "${_tree}")
endforeach()
qgc_test_pass("Android GStreamer_ROOT_DIR to the ABI's SDK tree")
