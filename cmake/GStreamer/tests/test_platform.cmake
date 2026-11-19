cmake_minimum_required(VERSION 3.25)
# The paths a platform's SDK is used from have no spaces.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Platform)

qgc_test_sandbox(_sandbox "platform${TEST_CASE}")

# pkg-config cannot pass on a path with spaces, so an SDK in one fails configure, except on iOS, which has no
# pkg-config. A download goes through the short path of a CPM_SOURCE_CACHE with spaces on a Windows host.
_qgc_gst_require_no_spaces(GStreamer_ROOT_DIR "C:/gstreamer/1.0/msvc_x86_64")
set(IOS TRUE)
_qgc_gst_require_no_spaces(GStreamer_ROOT_DIR "/Users/Jane Doe/GStreamer.xcframework")
set(IOS FALSE)

# Runs cmd's %~s for real, through its quoting, on a Windows host, before the stand-in below replaces it. Without short
# names on the volume, the long path comes back.
if(CMAKE_HOST_WIN32)
    set(_spaced "${_sandbox}/Jane Doe (x86) & co")
    file(MAKE_DIRECTORY "${_spaced}")
    _qgc_gst_windows_short_path("${_spaced}" _short)
    if(NOT IS_DIRECTORY "${_short}")
        message(FATAL_ERROR "ASSERT [short path]: '${_short}' for '${_spaced}' is not a directory")
    endif()
endif()

# Stands in for cmd's %~s with <_qgc_test_short_path>.
function(_qgc_gst_windows_short_path path out_var)
    set(${out_var}
        "${_qgc_test_short_path}"
        PARENT_SCOPE
    )
endfunction()
set(_qgc_test_short_path "${_sandbox}/JANEDO~1")
file(MAKE_DIRECTORY "${_qgc_test_short_path}")

if(TEST_CASE STREQUAL "spaces")
    _qgc_gst_require_no_spaces(GStreamer_ROOT_DIR "C:/Program Files/gstreamer")
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
elseif(TEST_CASE MATCHES "^download-spaces-(unix|no-short-name)$")
    set(CMAKE_HOST_WIN32 FALSE)
    if(CMAKE_MATCH_1 STREQUAL "no-short-name")
        set(CMAKE_HOST_WIN32 TRUE)
        set(_qgc_test_short_path "${_sandbox}/Jane Doe")
    endif()
    set(CPM_SOURCE_CACHE "${_sandbox}/Jane Doe/.cache/CPM")
    _qgc_gst_space_free_download_cache()
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

# host is Windows|CPM_SOURCE_CACHE under the sandbox|expected CPM_SOURCE_CACHE under the sandbox
foreach(_case IN ITEMS "FALSE|cache|cache" "TRUE|cache|cache" "TRUE|Jane Doe/.cache/CPM|JANEDO~1")
    qgc_test_split("${_case}" CMAKE_HOST_WIN32 _cache _expected)
    set(CPM_SOURCE_CACHE "${_sandbox}/${_cache}")
    _qgc_gst_space_free_download_cache()
    qgc_test_assert_streq("download cache for ${_case}" "${_sandbox}/${_expected}" "${CPM_SOURCE_CACHE}")
endforeach()
qgc_test_pass("SDK paths with spaces")
