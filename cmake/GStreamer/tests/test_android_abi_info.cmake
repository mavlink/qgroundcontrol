cmake_minimum_required(VERSION 3.25)
# Each supported Android ABI maps to its cerbero SDK tree and link policy; an ABI without a cerbero SDK tree fails
# configure.
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Android.cmake")

# <ABI>|<cerbero dir>|<textrel error>|<16 KB pages, as Qt's android_16kb_pages>
set(_cases "armeabi-v7a|armv7|TRUE|FALSE" "arm64-v8a|arm64|FALSE|TRUE" "x86|x86|TRUE|FALSE" "x86_64|x86_64|FALSE|TRUE")
foreach(_case IN LISTS _cases)
    qgc_test_split("${_case}" _abi _dir _textrel _16kb_pages)
    _qgc_gst_android_abi_info(${_abi} _info)
    qgc_test_assert_streq("${_abi} dir" "${_dir}" "${_info_DIR}")
    qgc_test_assert_streq("${_abi} textrel" "${_textrel}" "${_info_NEEDS_TEXTREL_ERROR}")
    qgc_test_assert_streq("${_abi} 16 KB pages" "${_16kb_pages}" "${_info_16KB_PAGES}")
endforeach()

_qgc_gst_android_abi_info(mips _abi)
message(FATAL_ERROR "the mips ABI was unexpectedly accepted")
