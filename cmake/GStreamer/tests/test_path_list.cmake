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

set(_windows_sdk "${_sandbox}/Program Files/gstreamer/1.0/msvc_x86_64")
set(_pc_INCLUDE_DIRS "${_sandbox}/Program")
set(_pc_CFLAGS_OTHER
    "Files/gstreamer/1.0/msvc_x86_64/include/gstreamer-1.0"
    "Files/gstreamer/1.0/msvc_x86_64/include"
    "-DKEEP_ME"
)
set(_pc_LIBRARY_DIRS "${_sandbox}/Program")
set(_pc_LDFLAGS_OTHER
    "Files/gstreamer/1.0/msvc_x86_64/lib"
    "-Wl,keep-me"
)
file(MAKE_DIRECTORY
    "${_windows_sdk}/include/gstreamer-1.0"
    "${_windows_sdk}/include"
    "${_windows_sdk}/lib"
)

_gst_recover_split_pkgconfig_paths(_pc INCLUDE_DIRS CFLAGS_OTHER LIBRARY_DIRS LDFLAGS_OTHER)

qgc_test_assert_in_list("recovers first split include dir" "${_windows_sdk}/include/gstreamer-1.0" _pc_INCLUDE_DIRS)
qgc_test_assert_in_list("recovers second split include dir" "${_windows_sdk}/include" _pc_INCLUDE_DIRS)
qgc_test_assert_in_list("keeps real compile option" "-DKEEP_ME" _pc_CFLAGS_OTHER)
qgc_test_assert_not_in_list("removes split include suffix from options" "Files/gstreamer/1.0/msvc_x86_64/include" _pc_CFLAGS_OTHER)
qgc_test_assert_in_list("recovers split library dir" "${_windows_sdk}/lib" _pc_LIBRARY_DIRS)
qgc_test_assert_in_list("keeps real link option" "-Wl,keep-me" _pc_LDFLAGS_OTHER)
qgc_test_assert_not_in_list("removes split library suffix from options" "Files/gstreamer/1.0/msvc_x86_64/lib" _pc_LDFLAGS_OTHER)
qgc_test_pass("coalesce_existing_paths")
