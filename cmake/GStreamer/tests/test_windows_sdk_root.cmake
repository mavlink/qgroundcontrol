cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../Acquire.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../SdkState.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Windows.cmake")

qgc_test_sandbox(_sandbox "windows-sdk-root${TEST_CASE}")

# An SDK the user names in GStreamer_ROOT_DIR is never replaced by a download: one discovery can't use fails configure.
if(TEST_CASE STREQUAL "explicit-incomplete" OR TEST_CASE STREQUAL "explicit-old")
    set(GStreamer_ROOT_DIR "${_sandbox}/sdk")
    set(GStreamer_AUTO_DOWNLOADED FALSE)
    set(QGC_GST_REQUESTED_VERSION 1.28.4)
    set(CMAKE_CXX_COMPILER_ARCHITECTURE_ID x64)
    file(MAKE_DIRECTORY "${GStreamer_ROOT_DIR}/include/gstreamer-1.0/gst" "${GStreamer_ROOT_DIR}/lib/gstreamer-1.0")
    file(WRITE "${GStreamer_ROOT_DIR}/include/gstreamer-1.0/gst/gstversion.h"
         "#define GST_VERSION_MAJOR (1)\n#define GST_VERSION_MINOR (26)\n#define GST_VERSION_MICRO (0)\n"
    )
    if(TEST_CASE STREQUAL "explicit-old")
        file(WRITE "${GStreamer_ROOT_DIR}/bin/pkg-config.exe" "")
        file(WRITE "${GStreamer_ROOT_DIR}/lib/pkgconfig/gstreamer-1.0.pc" "")
    endif()
    _qgc_gst_discover_windows_sdk()
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

# An installed SDK is found through the GSTREAMER_1_0_ROOT_<ARCH> variable of the target architecture, else through
# GSTREAMER_1_0_ROOT_MSVC_<ARCH> in MSVC builds; a variable naming a missing directory is skipped.
file(MAKE_DIRECTORY "${_sandbox}/env/x86_64" "${_sandbox}/env/msvc_x86_64" "${_sandbox}/env/msvc_arm64")
set(ENV{GSTREAMER_1_0_ROOT_ARM64} "${_sandbox}/env/missing")
set(ENV{GSTREAMER_1_0_ROOT_MSVC_ARM64} "${_sandbox}/env/msvc_arm64")
set(ENV{GSTREAMER_1_0_ROOT_MSVC_X86_64} "${_sandbox}/env/msvc_x86_64")
# MSVC|architecture|GSTREAMER_1_0_ROOT_X86_64|expected root, directories under <sandbox>/env.
set(_installed_cases "ON|x86_64|x86_64|x86_64" "ON|x86_64||msvc_x86_64" "OFF|x86_64||" "ON|arm64|x86_64|msvc_arm64"
                     "OFF|arm64||"
)
foreach(_case IN LISTS _installed_cases)
    qgc_test_split("${_case}" MSVC _arch _x86_64_root _expected)
    if(_x86_64_root)
        set(ENV{GSTREAMER_1_0_ROOT_X86_64} "${_sandbox}/env/${_x86_64_root}")
    else()
        unset(ENV{GSTREAMER_1_0_ROOT_X86_64})
    endif()
    if(_expected)
        set(_expected "${_sandbox}/env/${_expected}")
    endif()
    _qgc_gst_windows_installed_sdk(${_arch} _root)
    qgc_test_assert_streq("installed SDK for ${_case}" "${_expected}" "${_root}")
endforeach()
qgc_test_pass("Windows installed SDK lookup by architecture")

# compiler architecture id|generator platform|system processor|expected SDK architecture. The first
# case is Ninja from a vcvarsamd64_arm64 prompt, where only the compiler knows the target is ARM64.
set(_arch_cases "ARM64||AMD64|arm64" "x64|ARM64|ARM64|x86_64" "|ARM64|AMD64|arm64" "||aarch64|arm64" "||AMD64|x86_64")
foreach(_case IN LISTS _arch_cases)
    qgc_test_split(
        "${_case}" CMAKE_CXX_COMPILER_ARCHITECTURE_ID CMAKE_GENERATOR_PLATFORM CMAKE_SYSTEM_PROCESSOR _expected
    )
    _qgc_gst_windows_sdk_arch(_arch)
    qgc_test_assert_streq("SDK architecture for ${_case}" "${_expected}" "${_arch}")
endforeach()
qgc_test_pass("Windows SDK architecture follows the compiler target")
