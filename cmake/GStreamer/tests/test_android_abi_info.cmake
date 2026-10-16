cmake_minimum_required(VERSION 3.25)
# An Android ABI without a cerbero SDK tree fails configure.
include("${CMAKE_CURRENT_LIST_DIR}/../platform/Android.cmake")

_qgc_gst_android_abi_info(mips _abi)
message(FATAL_ERROR "the mips ABI was unexpectedly accepted")
