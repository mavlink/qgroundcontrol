cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Download)

# Every case uses its own VERSION so parallel test processes sharing CMAKE_BINARY_DIR
# never read or write the same pre-seeded sidecar.
set(_platform android)

# Pre-seeds the cached .sha256sum sidecar gstreamer_fetch_checksum reads for <version>.
function(_qgc_test_seed_sidecar version hash)
    gstreamer_get_package_url(${_platform} ${version} _url)
    string(SHA256 _url_hash "${_url}.sha256sum")
    file(WRITE "${CMAKE_BINARY_DIR}/_deps/checksums/${_url_hash}.sha256sum" "${hash}  archive.tar.xz\n")
endfunction()

# Pins <hash> for <version> in a stand-in build-config.json.
function(_qgc_test_set_pin version hash)
    set(QGC_BUILD_CONFIG_CONTENT
        "{\"gstreamer\":{\"checksums\":{\"${version}\":{\"${_platform}\":\"${hash}\"}}}}"
        PARENT_SCOPE
    )
endfunction()

string(SHA256 _pinned "pinned")
string(SHA256 _other "other")

if(TEST_CASE STREQUAL "sidecar-only-required")
    # Nothing is seeded: the check must fail before it would fetch the upstream sidecar.
    set(QGC_BUILD_CONFIG_CONTENT "{}")
    set(GStreamer_REQUIRE_CHECKSUM ON)
    gstreamer_fetch_checksum(${_platform} 9.0.4 _hash)
elseif(DEFINED TEST_CASE)
    message(FATAL_ERROR "Unknown test case: ${TEST_CASE}")
endif()

if(DEFINED TEST_CASE)
    message(FATAL_ERROR "${TEST_CASE} unexpectedly succeeded")
endif()

# A pinned SDK never consults the sidecar, so even a disagreeing cached one is ignored.
_qgc_test_set_pin(9.0.1 "${_pinned}")
_qgc_test_seed_sidecar(9.0.1 "${_other}")
gstreamer_fetch_checksum(${_platform} 9.0.1 _hash)
qgc_test_assert_streq("pin" "SHA256=${_pinned}" "${_hash}")
qgc_test_pass("pinned checksum is authoritative")

set(QGC_BUILD_CONFIG_CONTENT "{}")
set(GStreamer_REQUIRE_CHECKSUM OFF)
_qgc_test_seed_sidecar(9.0.2 "${_other}")
gstreamer_fetch_checksum(${_platform} 9.0.2 _hash)
qgc_test_assert_streq("unpinned sidecar" "SHA256=${_other}" "${_hash}")
qgc_test_pass("sidecar accepted only when checksums are optional")
