cmake_minimum_required(VERSION 3.25)
# Android builds on Windows and macOS hosts use the pkg-config the SDK bundles for that host; Linux hosts use their own,
# and a macOS host falls back to Homebrew's for an SDK without one.
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Android.cmake")

qgc_test_sandbox(_sandbox android-pkg-config)
set(_tools share/gst-android/ndk-build/tools)
file(WRITE "${_sandbox}/sdk/${_tools}/windows/pkg-config.exe" "")
file(WRITE "${_sandbox}/sdk/${_tools}/macos/pkg-config" "")
file(MAKE_DIRECTORY "${_sandbox}/older-sdk")

# root|host|expected pkg-config, paths under the sandbox
set(_cases "sdk|windows|sdk/${_tools}/windows/pkg-config.exe" "sdk|macos|sdk/${_tools}/macos/pkg-config" "sdk|linux|"
           "older-sdk|macos|"
)
foreach(_case IN LISTS _cases)
    qgc_test_split("${_case}" _root _host _expected)
    if(_expected)
        set(_expected "${_sandbox}/${_expected}")
    endif()
    _qgc_gst_android_sdk_pkg_config("${_sandbox}/${_root}" ${_host} _pkg_config)
    qgc_test_assert_streq("${_case}" "${_expected}" "${_pkg_config}")
endforeach()
qgc_test_pass("Android SDK pkg-config per host")
