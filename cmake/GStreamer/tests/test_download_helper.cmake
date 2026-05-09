# Test: qgc_resilient_download cache-hit + checksum branches, and
# qgc_parse_expected_hash validation. Network paths aren't exercised in
# script mode — full download flow is covered by per-platform configures.

cmake_minimum_required(VERSION 3.22)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../../modules")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Download)

set(_sandbox "${CMAKE_BINARY_DIR}/qgc_download_test")
file(REMOVE_RECURSE "${_sandbox}")
file(MAKE_DIRECTORY "${_sandbox}")

# ── parse_expected_hash: happy path ─────────────────────────────────────────
qgc_parse_expected_hash("SHA256=abcdef" _algo _hash)
qgc_test_assert_streq("algo" "SHA256" "${_algo}")
qgc_test_assert_streq("hash" "abcdef" "${_hash}")
qgc_test_pass("parse_expected_hash valid")

# ── cache hit, no hash ──────────────────────────────────────────────────────
file(WRITE "${_sandbox}/cached.txt" "hello world")
qgc_resilient_download(
    FILENAME        cached.txt
    DESTINATION_DIR "${_sandbox}"
    RESULT_VAR      _r1
    URLS            "https://invalid.example.invalid/never.txt"
    LOG_TAG         test
)
qgc_test_assert_streq("cache hit no-hash" "${_sandbox}/cached.txt" "${_r1}")
qgc_test_pass("cache hit no-hash")

# ── cache hit, valid hash ──────────────────────────────────────────────────
file(SHA256 "${_sandbox}/cached.txt" _real_hash)
qgc_resilient_download(
    FILENAME        cached.txt
    DESTINATION_DIR "${_sandbox}"
    RESULT_VAR      _r2
    URLS            "https://invalid.example.invalid/never.txt"
    EXPECTED_HASH   "SHA256=${_real_hash}"
    LOG_TAG         test
)
qgc_test_assert_streq("cache hit valid hash" "${_sandbox}/cached.txt" "${_r2}")
qgc_test_pass("cache hit valid hash")

# ── ALLOW_FAILURE keeps configure alive when every URL fails ────────────────
qgc_resilient_download(
    FILENAME        nonexistent.bin
    DESTINATION_DIR "${_sandbox}"
    RESULT_VAR      _r3
    URLS            "https://invalid.example.invalid/missing.bin"
    LOG_TAG         test
    ALLOW_FAILURE
    TIMEOUT         1
    INACTIVITY_TIMEOUT 1
)
qgc_test_assert_streq("ALLOW_FAILURE empty result" "" "${_r3}")
qgc_test_pass("ALLOW_FAILURE")

file(REMOVE_RECURSE "${_sandbox}")
