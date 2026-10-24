cmake_minimum_required(VERSION 3.25)
# The Orchestrator and the install dispatcher call each platform's helpers by name, and build-config.json must have the
# platform's version and plugin keys: check every platform, including the ones this host cannot build.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Platform)
include(Install)
foreach(_file IN ITEMS Apple Windows Linux Android MacOS IOS)
    include("${CMAKE_CURRENT_LIST_DIR}/../platform/${_file}.cmake")
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/../../modules/BuildConfigReader.cmake")

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

file(READ "${CMAKE_CURRENT_LIST_DIR}/../../../.github/build-config.json" QGC_BUILD_CONFIG_CONTENT)

# platform variable|platform|installs an SDK
foreach(
    _case IN ITEMS "ANDROID|android|FALSE" "IOS|ios|FALSE" "WIN32|windows|TRUE" "MACOS|macos|TRUE" "LINUX|linux|TRUE"
)
    qgc_test_split("${_case}" _variable _expected _installs)
    foreach(_platform_variable IN ITEMS ANDROID IOS WIN32 MACOS LINUX)
        set(${_platform_variable} FALSE)
    endforeach()
    set(${_variable} TRUE)

    qgc_gst_platform(_platform)
    qgc_test_assert_streq("${_variable} platform" "${_expected}" "${_platform}")
    if(NOT COMMAND _qgc_gst_discover_${_platform}_sdk)
        message(FATAL_ERROR "ASSERT [${_platform} discovery]: _qgc_gst_discover_${_platform}_sdk is not defined")
    endif()
    if(_installs AND NOT COMMAND gstreamer_install_${_platform}_sdk)
        message(FATAL_ERROR "ASSERT [${_platform} install]: gstreamer_install_${_platform}_sdk is not defined")
    endif()
    qgc_config_read(_version OPTIONAL gstreamer version ${_platform_VERSION_KEY})
    if(NOT _version)
        message(FATAL_ERROR "ASSERT [${_platform} version]: no gstreamer.version.${_platform_VERSION_KEY}")
    endif()
    qgc_config_read(_plugins LIST OPTIONAL gstreamer plugins ${_platform_PLUGIN_KEY})
    if(NOT _plugins)
        message(FATAL_ERROR "ASSERT [${_platform} plugins]: no gstreamer.plugins.${_platform_PLUGIN_KEY}")
    endif()
endforeach()
qgc_test_pass("every platform's helpers, version and plugin keys exist")
