cmake_minimum_required(VERSION 3.25)
# Drives qgc_gst_acquire_sdk offline: the archive is pre-seeded in the cache and pinned, so the
# download step is a verified cache hit and only locking, extraction and stamping are exercised.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/..")
include("${CMAKE_CURRENT_LIST_DIR}/_assert.cmake")
include(Download)

qgc_test_sandbox(_sandbox sdk-acquire)
set(CPM_SOURCE_CACHE "${_sandbox}/cache")
set(_version 9.9.1)
set(_cache_dir "${CPM_SOURCE_CACHE}/gstreamer-test-${_version}")
set(_content "${_cache_dir}/sdk")

# Exits non-zero when it can take the lock passed as LOCK, i.e. when nobody else holds it.
file(WRITE "${_sandbox}/try_lock.cmake" "file(LOCK \"\${LOCK}\" TIMEOUT 0 RESULT_VARIABLE _result)\n"
                                        "if(_result EQUAL 0)\n    message(FATAL_ERROR \"lock was free\")\nendif()\n"
)

# Packs <marker> as payload/marker.txt into the cached archive and pins its hash.
function(_qgc_test_seed_archive marker)
    file(REMOVE_RECURSE "${_sandbox}/src")
    file(WRITE "${_sandbox}/src/payload/marker.txt" "${marker}")
    file(MAKE_DIRECTORY "${_cache_dir}")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cJf "${_cache_dir}/sdk.tar.xz" payload
                    WORKING_DIRECTORY "${_sandbox}/src" COMMAND_ERROR_IS_FATAL ANY
    )
    file(SHA256 "${_cache_dir}/sdk.tar.xz" _hash)
    set(QGC_BUILD_CONFIG_CONTENT
        "{\"gstreamer\":{\"checksums\":{\"${_version}\":{\"android\":\"${_hash}\"}}}}"
        PARENT_SCOPE
    )
endfunction()

# qgc_gst_acquire_sdk extractor: records the call, checks the cache is locked and unpacks the archive.
# cmake-lint: disable=E1126
function(_qgc_test_extract staging archives)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" "-DLOCK=${_cache_dir}/.lock" -P "${_sandbox}/try_lock.cmake"
        RESULT_VARIABLE _lock_probe
        OUTPUT_QUIET ERROR_QUIET
    )
    if(NOT _lock_probe EQUAL 0)
        message(FATAL_ERROR "ASSERT [cache lock]: another process could lock the cache during extraction")
    endif()
    set_property(GLOBAL APPEND PROPERTY _qgc_test_extractions "${staging}")
    file(ARCHIVE_EXTRACT INPUT "${archives}" DESTINATION "${staging}")
endfunction()

# Acquires the SDK and asserts the published tree holds <marker> after <extractions> extractions.
function(_qgc_test_acquire label marker extractions)
    qgc_gst_acquire_sdk(
        PLATFORM
        android
        VERSION
        ${_version}
        CACHE_SUBDIR
        "gstreamer-test-${_version}"
        ARCHIVES
        sdk.tar.xz
        CONTENT
        sdk
        EXTRACT_FUNCTION
        _qgc_test_extract
        CONTENT_DIR_OUT
        _dir
    )
    qgc_test_assert_streq("${label}: content dir" "${_content}" "${_dir}")
    get_property(_extracted GLOBAL PROPERTY _qgc_test_extractions)
    list(LENGTH _extracted _count)
    qgc_test_assert_streq("${label}: extractions" "${extractions}" "${_count}")
    file(READ "${_content}/payload/marker.txt" _marker)
    qgc_test_assert_streq("${label}: payload" "${marker}" "${_marker}")
    if(NOT EXISTS "${_content}/.complete" OR EXISTS "${_content}.partial")
        message(FATAL_ERROR "ASSERT [${label}]: expected a stamped tree and no staging directory")
    endif()
endfunction()

_qgc_test_seed_archive(first)
_qgc_test_acquire("first acquisition" first 1)
_qgc_test_acquire("unchanged archive is reused" first 1)
qgc_test_pass("acquire extracts once")

# Each way a published tree can be stale or missing forces a fresh extraction.
set(_extractions 1)
foreach(_damage IN ITEMS other-hash missing-stamp interrupted)
    if(_damage STREQUAL "other-hash")
        file(WRITE "${_content}/.complete" "sdk.tar.xz=SHA256=0\n")
    elseif(_damage STREQUAL "missing-stamp")
        file(REMOVE "${_content}/.complete")
    else()
        file(REMOVE_RECURSE "${_content}")
        file(WRITE "${_content}.partial/payload/marker.txt" "half-written")
    endif()
    math(EXPR _extractions "${_extractions} + 1")
    _qgc_test_acquire("${_damage}" first ${_extractions})
endforeach()
qgc_test_pass("stale or interrupted extractions are redone")

_qgc_test_seed_archive(second)
math(EXPR _extractions "${_extractions} + 1")
_qgc_test_acquire("re-pinned archive" second ${_extractions})
qgc_test_pass("archive change re-extracts")

# A tree stamped with the pinned checksum is used without its archive, which CI caches leave out.
function(qgc_resilient_download)
    message(FATAL_ERROR "ASSERT [pinned tree reused]: tried to download ${ARGV}")
endfunction()
file(REMOVE "${_cache_dir}/sdk.tar.xz")
_qgc_test_acquire("archive not cached" second ${_extractions})
qgc_test_pass("a pinned tree is used without its archive")

# Either way the SDK's notice and SBOM entry name the package it came from.
gstreamer_get_package_url(android ${_version} _url)
gstreamer_get_fallback_checksum(android ${_version} _pin)
string(REPLACE "SHA256=" "" _pin "${_pin}")
qgc_test_assert_streq("recorded package" "${_url}|${_pin}" "$CACHE{QGC_GST_RESOLVED_PACKAGES}")
qgc_test_pass("acquired package recorded")
