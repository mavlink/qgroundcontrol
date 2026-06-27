cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Install)

set(_sandbox "${CMAKE_BINARY_DIR}/qgc_windows_runtime_libproxy_sandbox")
file(REMOVE_RECURSE "${_sandbox}")
file(MAKE_DIRECTORY "${_sandbox}/bin" "${_sandbox}/lib/libproxy")
file(TOUCH "${_sandbox}/bin/proxy-1.dll")
file(TOUCH "${_sandbox}/lib/libproxy/pxbackend-1.0.dll")

_gstreamer_windows_runtime_dependency_dirs("${_sandbox}" _runtime_dirs)

qgc_test_assert_in_list(
    "includes libproxy backend dll directory"
    "${_sandbox}/lib/libproxy"
    _runtime_dirs
)

file(REMOVE_RECURSE "${_sandbox}")
qgc_test_pass("windows runtime libproxy dependency dirs")
