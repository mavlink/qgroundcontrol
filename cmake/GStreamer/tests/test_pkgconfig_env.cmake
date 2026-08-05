cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(PkgConfig)

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_sep ";")
else()
    set(_sep ":")
endif()

set(ENV{PKG_CONFIG_PATH} "/system/lib/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "")
gstreamer_apply_pkgconfig_env(
    MODE SDK
    LIBDIR "/sdk/lib/pkgconfig" "/sdk/lib/gstreamer-1.0/pkgconfig"
)
qgc_test_assert_streq("SDK clears PATH" "" "$ENV{PKG_CONFIG_PATH}")
qgc_test_assert_streq("SDK sets LIBDIR"
    "/sdk/lib/pkgconfig${_sep}/sdk/lib/gstreamer-1.0/pkgconfig"
    "$ENV{PKG_CONFIG_LIBDIR}")
qgc_test_pass("SDK mode")

set(ENV{PKG_CONFIG} "")
unset(PKG_CONFIG_EXECUTABLE CACHE)
gstreamer_apply_pkgconfig_env(
    MODE SDK
    PKG_CONFIG_EXE "/opt/sdk/bin/pkg-config"
    LIBDIR "/sdk/lib/pkgconfig"
)
qgc_test_assert_streq("PKG_CONFIG env" "/opt/sdk/bin/pkg-config" "$ENV{PKG_CONFIG}")
qgc_test_assert_streq("PKG_CONFIG_EXECUTABLE cache" "/opt/sdk/bin/pkg-config" "${PKG_CONFIG_EXECUTABLE}")
qgc_test_pass("PKG_CONFIG_EXE forwarding")

set(ENV{PKG_CONFIG_PATH} "/system/lib/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "")
gstreamer_apply_pkgconfig_env(
    MODE SYSTEM_AUGMENT
    LIBDIR "/usr/lib/x86_64-linux-gnu/pkgconfig"
)
qgc_test_assert_streq("SYSTEM_AUGMENT prepends"
    "/usr/lib/x86_64-linux-gnu/pkgconfig${_sep}/system/lib/pkgconfig"
    "$ENV{PKG_CONFIG_PATH}")
qgc_test_pass("SYSTEM_AUGMENT prepend")

set(ENV{PKG_CONFIG_PATH} "")
gstreamer_apply_pkgconfig_env(
    MODE SYSTEM_AUGMENT
    LIBDIR "/sdk/lib/pkgconfig"
)
qgc_test_assert_streq("SYSTEM_AUGMENT empty path" "/sdk/lib/pkgconfig" "$ENV{PKG_CONFIG_PATH}")
qgc_test_pass("SYSTEM_AUGMENT empty path")

set(PKG_CONFIG_ARGN "--static")
gstreamer_apply_pkgconfig_env(
    MODE SDK
    LIBDIR "/sdk/lib/pkgconfig"
    DONT_DEFINE_PREFIX
)
qgc_test_assert_in_list("ARGN gained --dont-define-prefix" "--dont-define-prefix" PKG_CONFIG_ARGN)
qgc_test_assert_in_list("ARGN preserved --static"          "--static"             PKG_CONFIG_ARGN)
list(LENGTH PKG_CONFIG_ARGN _n_after_first)
qgc_test_assert_streq("ARGN length after first call" "2" "${_n_after_first}")

gstreamer_apply_pkgconfig_env(
    MODE SDK
    LIBDIR "/sdk/lib/pkgconfig"
    DONT_DEFINE_PREFIX
)
list(LENGTH PKG_CONFIG_ARGN _n_after_second)
qgc_test_assert_streq("ARGN idempotent" "2" "${_n_after_second}")
qgc_test_pass("DONT_DEFINE_PREFIX idempotent")

set(ENV{PKG_CONFIG_PATH} "")
set(ENV{PKG_CONFIG_LIBDIR} "")
set(ENV{PKG_CONFIG} "")
