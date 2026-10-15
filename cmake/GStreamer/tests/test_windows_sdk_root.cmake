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

file(REMOVE_RECURSE "${_sandbox}")
qgc_test_pass("Windows extracted SDK root selection")

# The installer is staged into <dir>.partial and renamed, so anything it records about its install dir
# (environment variable, registry, uninstaller) would point at a dead path. This stand-in installs a
# complete SDK into /DIR and, like cerbero's Inno script, records /DIR unless run in portable mode.
if(CMAKE_HOST_UNIX)
    set(_installer "${_sandbox}/installer/gstreamer-x86_64.exe")
    set(_recorded "${_sandbox}/recorded-install-dir.txt")
    file(
        WRITE "${_sandbox}/installer/stage/gstreamer-x86_64.exe"
        "#!/bin/sh\n"
        "portable=0\n"
        "for arg in \"$@\"; do\n"
        "    case \"$arg\" in\n"
        "        /DIR=*) dir=\"\${arg#/DIR=}\" ;;\n"
        "        /portable=1) portable=1 ;;\n"
        "    esac\n"
        "done\n"
        "mkdir -p \"$dir/bin\" \"$dir/include/gstreamer-1.0\" \"$dir/lib/gstreamer-1.0\" \"$dir/lib/pkgconfig\"\n"
        ": > \"$dir/bin/pkg-config.exe\"\n"
        ": > \"$dir/lib/pkgconfig/gstreamer-1.0.pc\"\n"
        "if [ \"$portable\" != 1 ]; then echo \"$dir\" > '${_recorded}'; fi\n"
    )
    file(
        COPY "${_sandbox}/installer/stage/gstreamer-x86_64.exe"
        DESTINATION "${_sandbox}/installer"
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )
    set(GStreamer_REQUIRE_CHECKSUM ON)
    _qgc_gst_windows_extract("${_sandbox}/sdk/install.partial" "${_installer}")
    _qgc_gst_windows_sdk_complete("${_sandbox}/sdk/install.partial" _complete)
    qgc_test_assert_streq("installed SDK is complete" TRUE "${_complete}")
    if(EXISTS "${_recorded}")
        file(READ "${_recorded}" _recorded_dir)
        message(FATAL_ERROR "ASSERT [installer side effects]: the installer recorded '${_recorded_dir}'")
    endif()
    file(REMOVE_RECURSE "${_sandbox}")
    qgc_test_pass("Windows SDK installer records nothing outside its install dir")
endif()

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
