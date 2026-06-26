# Test: _gst_coalesce_existing_paths rejoins path tokens split at spaces.
cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Link)

set(_sandbox "${CMAKE_CURRENT_BINARY_DIR}/path list sandbox")
set(_include_dir "${_sandbox}/sdk root/include/gstreamer-1.0")
file(MAKE_DIRECTORY "${_include_dir}")

set(_paths
    "${CMAKE_CURRENT_BINARY_DIR}/path"
    "list"
    "sandbox/sdk"
    "root/include/gstreamer-1.0"
    "/missing/prefix"
)

_gst_coalesce_existing_paths(_paths)

qgc_test_assert_in_list("coalesces split path" "${_include_dir}" _paths)
qgc_test_assert_not_in_list("removes first split token" "${CMAKE_CURRENT_BINARY_DIR}/path" _paths)
qgc_test_assert_not_in_list("removes middle split token" "list" _paths)
qgc_test_assert_in_list("keeps unresolved token for caller diagnostics" "/missing/prefix" _paths)
qgc_test_pass("coalesce_existing_paths")
