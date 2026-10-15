cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Windows.cmake")

qgc_test_sandbox(_sandbox windows-sdk-root)

# Creates an empty <dir>/bin/pkg-config.exe under the sandbox.
function(_qgc_test_touch_pkg_config dir)
    file(WRITE "${_sandbox}/${dir}/bin/pkg-config.exe" "")
endfunction()

_qgc_test_touch_pkg_config(direct)
_qgc_gst_find_windows_sdk_root("${_sandbox}/direct" _root)
qgc_test_assert_streq("pkg-config directly under root" "${_sandbox}/direct" "${_root}")

# The installer nests the SDK under 1.0/<arch>; a bundled helper pkg-config deeper in the
# tree must not win.
_qgc_test_touch_pkg_config(nested/1.0/msvc_x86_64)
_qgc_test_touch_pkg_config(nested/1.0/msvc_x86_64/lib/gstreamer-1.0/tools)
_qgc_gst_find_windows_sdk_root("${_sandbox}/nested" _root)
qgc_test_assert_streq("shortest pkg-config path" "${_sandbox}/nested/1.0/msvc_x86_64" "${_root}")

_qgc_test_touch_pkg_config(tie/Zeta)
_qgc_test_touch_pkg_config(tie/alpha)
_qgc_gst_find_windows_sdk_root("${_sandbox}/tie" _root)
qgc_test_assert_streq("equal depth tiebreak ignores case" "${_sandbox}/tie/alpha" "${_root}")

file(MAKE_DIRECTORY "${_sandbox}/empty")
_qgc_gst_find_windows_sdk_root("${_sandbox}/empty" _root)
qgc_test_assert_streq("no pkg-config" "" "${_root}")
qgc_test_pass("Windows extracted SDK root selection")

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
    string(REPLACE "|" ";" _parts "${_case}")
    list(GET _parts 0 MSVC)
    list(GET _parts 1 _arch)
    list(GET _parts 2 _x86_64_root)
    list(GET _parts 3 _expected)
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
    string(REPLACE "|" ";" _parts "${_case}")
    list(GET _parts 0 CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    list(GET _parts 1 CMAKE_GENERATOR_PLATFORM)
    list(GET _parts 2 CMAKE_SYSTEM_PROCESSOR)
    list(GET _parts 3 _expected)
    _qgc_gst_windows_sdk_arch(_arch)
    qgc_test_assert_streq("SDK architecture for ${_case}" "${_expected}" "${_arch}")
endforeach()
qgc_test_pass("Windows SDK architecture follows the compiler target")
