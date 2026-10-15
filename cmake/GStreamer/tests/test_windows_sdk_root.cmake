cmake_minimum_required(VERSION 3.25)
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Windows.cmake")

set(_sandbox "${CMAKE_CURRENT_BINARY_DIR}/windows-sdk-root")
file(REMOVE_RECURSE "${_sandbox}")

# Creates an empty <dir>/bin/pkg-config.exe under the sandbox.
function(_qgc_test_touch_pkg_config dir)
    file(WRITE "${_sandbox}/${dir}/bin/pkg-config.exe" "")
endfunction()

_qgc_test_touch_pkg_config(direct)
_qgc_find_win_sdk_root("${_sandbox}/direct" _root)
qgc_test_assert_streq("pkg-config directly under root" "${_sandbox}/direct" "${_root}")

# The installer nests the SDK under 1.0/<arch>; a bundled helper pkg-config deeper in the
# tree must not win.
_qgc_test_touch_pkg_config(nested/1.0/msvc_x86_64)
_qgc_test_touch_pkg_config(nested/1.0/msvc_x86_64/lib/gstreamer-1.0/tools)
_qgc_find_win_sdk_root("${_sandbox}/nested" _root)
qgc_test_assert_streq("shortest pkg-config path" "${_sandbox}/nested/1.0/msvc_x86_64" "${_root}")

_qgc_test_touch_pkg_config(tie/Zeta)
_qgc_test_touch_pkg_config(tie/alpha)
_qgc_find_win_sdk_root("${_sandbox}/tie" _root)
qgc_test_assert_streq("equal depth tiebreak ignores case" "${_sandbox}/tie/alpha" "${_root}")

file(MAKE_DIRECTORY "${_sandbox}/empty")
_qgc_find_win_sdk_root("${_sandbox}/empty" _root)
qgc_test_assert_streq("no pkg-config" "" "${_root}")

file(REMOVE_RECURSE "${_sandbox}")
qgc_test_pass("Windows extracted SDK root selection")
