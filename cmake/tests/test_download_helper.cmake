cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../modules")
include("${CMAKE_CURRENT_LIST_DIR}/../GStreamer/tests/_assert.cmake")
include(Download)

qgc_test_sandbox(_sandbox qgc_download_test)

qgc_parse_expected_hash("SHA256=AbCdEf" _algo _hash)
qgc_test_assert_streq("algo" "SHA256" "${_algo}")
qgc_test_assert_streq("hash, lowercased" "abcdef" "${_hash}")
qgc_test_pass("parse_expected_hash valid")

file(WRITE "${_sandbox}/cached.txt" "hello world")
qgc_resilient_download(
    FILENAME
    cached.txt
    DESTINATION_DIR
    "${_sandbox}"
    RESULT_VAR
    _r1
    URLS
    "https://invalid.example.invalid/never.txt"
    LOG_TAG
    test
)
qgc_test_assert_streq("cache hit no-hash" "${_sandbox}/cached.txt" "${_r1}")
qgc_test_pass("cache hit no-hash")

file(SHA256 "${_sandbox}/cached.txt" _real_hash)
qgc_resilient_download(
    FILENAME
    cached.txt
    DESTINATION_DIR
    "${_sandbox}"
    RESULT_VAR
    _r2
    URLS
    "https://invalid.example.invalid/never.txt"
    EXPECTED_HASH
    "SHA256=${_real_hash}"
    LOG_TAG
    test
)
qgc_test_assert_streq("cache hit valid hash" "${_sandbox}/cached.txt" "${_r2}")
qgc_test_pass("cache hit valid hash")

string(TOUPPER "${_real_hash}" _real_hash_upper)
qgc_resilient_download(
    FILENAME
    cached.txt
    DESTINATION_DIR
    "${_sandbox}"
    RESULT_VAR
    _r2_upper
    URLS
    "https://invalid.example.invalid/never.txt"
    EXPECTED_HASH
    "SHA256=${_real_hash_upper}"
    LOG_TAG
    test
    ALLOW_FAILURE
    TIMEOUT
    1
    INACTIVITY_TIMEOUT
    1
    RETRIES
    1
    RETRY_BACKOFF
    0
)
qgc_test_assert_streq("cache hit uppercase hash" "${_sandbox}/cached.txt" "${_r2_upper}")
qgc_test_pass("cache hit uppercase hash")

qgc_resilient_download(
    FILENAME
    nonexistent.bin
    DESTINATION_DIR
    "${_sandbox}"
    RESULT_VAR
    _r3
    URLS
    "https://invalid.example.invalid/missing.bin"
    LOG_TAG
    test
    ALLOW_FAILURE
    TIMEOUT
    1
    INACTIVITY_TIMEOUT
    1
    RETRIES
    1
    RETRY_BACKOFF
    0
)
qgc_test_assert_streq("ALLOW_FAILURE empty result" "" "${_r3}")
qgc_test_pass("ALLOW_FAILURE")

# The checksum applies to whichever URL serves the file: a fallback URL is not trusted more than the first one.
file(WRITE "${_sandbox}/served/good.bin" "good")
file(WRITE "${_sandbox}/served/bad.bin" "bad")
file(SHA256 "${_sandbox}/served/good.bin" _good_hash)
foreach(_case IN ITEMS "good|${_sandbox}/downloads/good.bin" "bad|")
    string(REPLACE "|" ";" _case "${_case}")
    list(GET _case 0 _served)
    list(GET _case 1 _expected)
    qgc_resilient_download(
        FILENAME
        ${_served}.bin
        DESTINATION_DIR
        "${_sandbox}/downloads"
        RESULT_VAR
        _r4
        URLS
        "file://${_sandbox}/served/missing.bin"
        "file://${_sandbox}/served/${_served}.bin"
        EXPECTED_HASH
        "SHA256=${_good_hash}"
        LOG_TAG
        test
        ALLOW_FAILURE
        RETRIES
        1
        RETRY_BACKOFF
        0
    )
    qgc_test_assert_streq("fallback URL serving ${_served} content" "${_expected}" "${_r4}")
endforeach()
qgc_test_pass("fallback URL verified against EXPECTED_HASH")
