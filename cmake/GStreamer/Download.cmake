# GStreamer SDK download / URL / checksum helpers.

# Absolute path disambiguates from this file (also named Download.cmake): when
# test_download.cmake adds cmake/GStreamer/ to MODULE_PATH, a bare `include(Download)`
# would resolve to this file and self-recurse.
include("${CMAKE_CURRENT_LIST_DIR}/../modules/Download.cmake")  # qgc_resilient_download / qgc_parse_expected_hash

function(gstreamer_get_package_url PLATFORM VERSION OUTPUT_VAR)
    set(_base "https://gstreamer.freedesktop.org")

    # URL templates for simple platforms — VERSION substituted via _base.
    set(_url_android       "${_base}/data/pkg/android/${VERSION}/gstreamer-1.0-android-universal-${VERSION}.tar.xz")
    set(_url_ios           "${_base}/data/pkg/ios/${VERSION}/gstreamer-1.0-devel-${VERSION}-ios-universal.pkg")
    set(_url_macos         "${_base}/data/pkg/macos/${VERSION}/gstreamer-1.0-${VERSION}-universal.pkg")
    set(_url_macos_devel   "${_base}/data/pkg/macos/${VERSION}/gstreamer-1.0-devel-${VERSION}-universal.pkg")

    if(PLATFORM MATCHES "^windows_msvc_(x64|arm64)$")
        # Windows: 1.28+ ships .exe with arch-tagged filename (defensive guard against manual override).
        if(VERSION VERSION_LESS "1.28.0")
            message(FATAL_ERROR
                "Windows GStreamer SDK requires version 1.28 or later (got '${VERSION}'). "
                "Bump gstreamer.version.windows in build-config.json.")
        endif()
        if(CMAKE_MATCH_1 STREQUAL "x64")
            set(_arch "x86_64")
        else()
            set(_arch "arm64")
        endif()
        set(_url "${_base}/data/pkg/windows/${VERSION}/msvc/gstreamer-1.0-msvc-${_arch}-${VERSION}.exe")
    elseif(DEFINED _url_${PLATFORM})
        set(_url "${_url_${PLATFORM}}")
    else()
        message(FATAL_ERROR "gstreamer_get_package_url: Unknown platform '${PLATFORM}'")
    endif()

    set(${OUTPUT_VAR} "${_url}" PARENT_SCOPE)
endfunction()

function(gstreamer_get_s3_mirror_url PLATFORM VERSION OUTPUT_VAR)
    set(_s3_base "https://qgroundcontrol.s3.us-west-2.amazonaws.com/dependencies/gstreamer")

    if(PLATFORM STREQUAL "android")
        set(_dir "android")
    elseif(PLATFORM STREQUAL "ios")
        set(_dir "ios")
    elseif(PLATFORM MATCHES "^macos")
        set(_dir "macos")
    elseif(PLATFORM MATCHES "^windows_msvc_")
        set(_dir "windows")
    else()
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    gstreamer_get_package_url("${PLATFORM}" "${VERSION}" _primary_url)
    cmake_path(GET _primary_url FILENAME _filename)

    set(${OUTPUT_VAR} "${_s3_base}/${_dir}/${_filename}" PARENT_SCOPE)
endfunction()

function(gstreamer_get_fallback_checksum PLATFORM VERSION OUTPUT_VAR)
    set(_checksum "")

    if(DEFINED QGC_BUILD_CONFIG_CONTENT)
        string(JSON _hash ERROR_VARIABLE _err
            GET "${QGC_BUILD_CONFIG_CONTENT}" "gstreamer" "checksums" "${VERSION}" "${PLATFORM}")
        if(NOT _err AND _hash)
            set(_checksum "SHA256=${_hash}")
        endif()
    endif()

    set(${OUTPUT_VAR} "${_checksum}" PARENT_SCOPE)
endfunction()

function(_gstreamer_fallback_or_warn PLATFORM VERSION OUTPUT_VAR MESSAGE)
    gstreamer_get_fallback_checksum(${PLATFORM} ${VERSION} _fallback_hash)
    if(_fallback_hash)
        message(WARNING "GStreamer: ${MESSAGE} for ${PLATFORM} ${VERSION}; using pinned fallback checksum.")
        set(${OUTPUT_VAR} "${_fallback_hash}" PARENT_SCOPE)
        return()
    endif()
    if(GStreamer_REQUIRE_CHECKSUM)
        message(FATAL_ERROR
            "GStreamer: ${MESSAGE} for ${PLATFORM} ${VERSION} "
            "and no pinned fallback exists. Set GStreamer_REQUIRE_CHECKSUM=OFF to bypass.")
    endif()
    message(WARNING
        "GStreamer: ${MESSAGE} for ${PLATFORM} ${VERSION}; "
        "the SDK will be used WITHOUT integrity verification — supply-chain "
        "authenticity is NOT guaranteed. Only proceed on a trusted network/mirror. "
        "(GStreamer_REQUIRE_CHECKSUM=ON makes this a hard error.)")
    set(${OUTPUT_VAR} "" PARENT_SCOPE)
endfunction()

# _gstreamer_download_sidecar(<url> <dest_file> <result_var>)
# Downloads the .sha256sum sidecar to DEST_FILE; sets RESULT_VAR TRUE on success.
function(_gstreamer_download_sidecar URL DEST_FILE RESULT_VAR)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/_deps/checksums")
    set(_tmp "${DEST_FILE}.tmp")
    foreach(_attempt RANGE 1 3)
        file(DOWNLOAD "${URL}" "${_tmp}" STATUS _status TIMEOUT 30 INACTIVITY_TIMEOUT 10 TLS_VERIFY ON)
        list(GET _status 0 _code)
        if(_code EQUAL 0)
            file(RENAME "${_tmp}" "${DEST_FILE}")
            set(${RESULT_VAR} TRUE PARENT_SCOPE)
            return()
        endif()
        list(GET _status 1 _msg)
        message(STATUS "GStreamer: Checksum download attempt ${_attempt}/3 failed: ${_msg}")
        file(REMOVE "${_tmp}")
    endforeach()
    set(${RESULT_VAR} FALSE PARENT_SCOPE)
endfunction()

function(gstreamer_fetch_checksum PLATFORM VERSION OUTPUT_VAR)
    gstreamer_get_package_url(${PLATFORM} ${VERSION} _pkg_url)

    string(FIND "${_pkg_url}" "?" _qs_pos)
    if(NOT _qs_pos EQUAL -1)
        message(STATUS "GStreamer: Skipping checksum for ${PLATFORM} ${VERSION} (URL contains query string)")
        set(${OUTPUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    set(_checksum_url "${_pkg_url}.sha256sum")
    string(SHA256 _url_hash "${_checksum_url}")
    set(_checksum_file "${CMAKE_BINARY_DIR}/_deps/checksums/${_url_hash}.sha256sum")

    # In-tree pin is authoritative: a fetched sidecar is attacker-influenceable
    # (compromised mirror/MITM), so it can never silently override the pin.
    gstreamer_get_fallback_checksum(${PLATFORM} ${VERSION} _pinned_hash)

    set(_dl_ok FALSE)
    foreach(_round RANGE 1 2)
        if(NOT EXISTS "${_checksum_file}")
            _gstreamer_download_sidecar("${_checksum_url}" "${_checksum_file}" _dl_ok)
            if(NOT _dl_ok)
                _gstreamer_fallback_or_warn(${PLATFORM} ${VERSION} _result "Checksum sidecar unavailable")
                set(${OUTPUT_VAR} "${_result}" PARENT_SCOPE)
                return()
            endif()
        endif()

        file(READ "${_checksum_file}" _content)
        string(STRIP "${_content}" _content)
        string(REGEX MATCH "([0-9a-fA-F]{64})" _match "${_content}")
        if(_match)
            set(_fetched_hash "SHA256=${CMAKE_MATCH_1}")
            if(_pinned_hash AND NOT _pinned_hash STREQUAL _fetched_hash)
                message(FATAL_ERROR
                    "GStreamer: checksum mismatch for ${PLATFORM} ${VERSION} — the fetched "
                    ".sha256sum sidecar does NOT match the in-tree pinned hash. The pinned "
                    "hash is authoritative; a divergent sidecar indicates a compromised "
                    "mirror, MITM, or an out-of-date pin.\n"
                    "  pinned (authoritative): ${_pinned_hash}\n"
                    "  fetched (sidecar):      ${_fetched_hash}\n"
                    "If the upstream SDK legitimately changed, update the pinned hash in "
                    ".github/build-config.json (gstreamer.checksums.${VERSION}.${PLATFORM}).")
            endif()
            # Hand the authoritative pinned hash to the integrity check when present.
            if(_pinned_hash)
                set(${OUTPUT_VAR} "${_pinned_hash}" PARENT_SCOPE)
                return()
            endif()
            # No trusted pin: the same-origin sidecar proves nothing against a bad
            # mirror/MITM, so it must not satisfy GStreamer_REQUIRE_CHECKSUM=ON.
            if(GStreamer_REQUIRE_CHECKSUM)
                message(FATAL_ERROR
                    "GStreamer: no trusted (in-tree pinned) checksum for ${PLATFORM} ${VERSION}; "
                    "the fetched same-origin .sha256sum sidecar cannot satisfy "
                    "GStreamer_REQUIRE_CHECKSUM=ON. Pin the hash in .github/build-config.json "
                    "(gstreamer.checksums.${VERSION}.${PLATFORM}) or set GStreamer_REQUIRE_CHECKSUM=OFF.")
            endif()
            message(WARNING
                "GStreamer: using same-origin .sha256sum sidecar for ${PLATFORM} ${VERSION}; "
                "this detects corruption only, NOT supply-chain tampering. Pin the hash in "
                ".github/build-config.json for authenticity.")
            set(${OUTPUT_VAR} "${_fetched_hash}" PARENT_SCOPE)
            return()
        endif()

        message(STATUS "GStreamer: Could not parse checksum for ${PLATFORM} ${VERSION}")
        file(REMOVE "${_checksum_file}")
    endforeach()

    _gstreamer_fallback_or_warn(${PLATFORM} ${VERSION} _result "Checksum content invalid/unparseable")
    set(${OUTPUT_VAR} "${_result}" PARENT_SCOPE)
endfunction()

# GStreamer-specific wrapper around qgc_resilient_download (cmake/modules/Download.cmake).
# Adds the GStreamer log prefix, install hint, and honours legacy
# QGC_GST_DOWNLOAD_TIMEOUT / QGC_GST_DOWNLOAD_INACTIVITY_TIMEOUT cache vars
# (CI flake-fixer knobs that predate the project-level QGC_DOWNLOAD_* vars).
function(gstreamer_resilient_download)
    cmake_parse_arguments(ARG "ALLOW_FAILURE"
        "FILENAME;DESTINATION_DIR;RESULT_VAR;TIMEOUT;INACTIVITY_TIMEOUT;EXPECTED_HASH"
        "URLS" ${ARGN})
    if(NOT ARG_RESULT_VAR)
        message(FATAL_ERROR "gstreamer_resilient_download: RESULT_VAR is required")
    endif()
    if(NOT ARG_TIMEOUT AND DEFINED QGC_GST_DOWNLOAD_TIMEOUT)
        set(ARG_TIMEOUT ${QGC_GST_DOWNLOAD_TIMEOUT})
    endif()
    if(NOT ARG_INACTIVITY_TIMEOUT AND DEFINED QGC_GST_DOWNLOAD_INACTIVITY_TIMEOUT)
        set(ARG_INACTIVITY_TIMEOUT ${QGC_GST_DOWNLOAD_INACTIVITY_TIMEOUT})
    endif()

    # GStreamer SDKs are large (the Android universal tarball is ~1 GB); the module's
    # 120 s wall-clock default kills slow-but-healthy CI downloads mid-transfer. Use a
    # generous cap and let INACTIVITY_TIMEOUT (60 s) catch genuine stalls instead.
    # Note: this 1200 s default overrides the project-level QGC_DOWNLOAD_TIMEOUT for
    # GStreamer downloads — use QGC_GST_DOWNLOAD_TIMEOUT to tune the GStreamer cap.
    if(NOT ARG_TIMEOUT)
        set(ARG_TIMEOUT 1200)
    endif()
    if(NOT ARG_INACTIVITY_TIMEOUT)
        set(ARG_INACTIVITY_TIMEOUT 60)
    endif()

    set(_fwd
        FILENAME         "${ARG_FILENAME}"
        DESTINATION_DIR  "${ARG_DESTINATION_DIR}"
        RESULT_VAR       _qrd_result
        URLS             "${ARG_URLS}"
        LOG_TAG          "GStreamer"
        FAILURE_HINT     "Install manually from https://gstreamer.freedesktop.org/download/ or set GStreamer_ROOT_DIR."
    )
    if(ARG_TIMEOUT)
        list(APPEND _fwd TIMEOUT ${ARG_TIMEOUT})
    endif()
    if(ARG_INACTIVITY_TIMEOUT)
        list(APPEND _fwd INACTIVITY_TIMEOUT ${ARG_INACTIVITY_TIMEOUT})
    endif()
    if(ARG_EXPECTED_HASH)
        list(APPEND _fwd EXPECTED_HASH "${ARG_EXPECTED_HASH}")
    endif()
    if(ARG_ALLOW_FAILURE)
        list(APPEND _fwd ALLOW_FAILURE)
    endif()
    qgc_resilient_download(${_fwd})

    set(${ARG_RESULT_VAR} "${_qrd_result}" PARENT_SCOPE)
endfunction()

function(gstreamer_download_sdk PLATFORM VERSION FILENAME DESTINATION_DIR RESULT_VAR)
    cmake_parse_arguments(_DL "ALLOW_FAILURE" "" "" ${ARGN})

    gstreamer_get_package_url("${PLATFORM}" "${VERSION}" _url)
    gstreamer_get_s3_mirror_url("${PLATFORM}" "${VERSION}" _s3_url)
    gstreamer_fetch_checksum("${PLATFORM}" "${VERSION}" _hash)

    set(_urls "${_url}")
    if(_s3_url)
        list(APPEND _urls "${_s3_url}")
    endif()

    set(_args
        URLS ${_urls}
        FILENAME "${FILENAME}"
        DESTINATION_DIR "${DESTINATION_DIR}"
        RESULT_VAR _result
    )
    if(_hash)
        list(APPEND _args EXPECTED_HASH "${_hash}")
    endif()
    if(_DL_ALLOW_FAILURE)
        list(APPEND _args ALLOW_FAILURE)
    endif()

    gstreamer_resilient_download(${_args})
    set(${RESULT_VAR} "${_result}" PARENT_SCOPE)
endfunction()

# gstreamer_resolve_or_download_sdk
# Shared prologue for Windows/macOS/iOS: selects cache dir from CPM_SOURCE_CACHE
# or CMAKE_BINARY_DIR, downloads the SDK package(s) if not already cached, then
# calls VALIDATE_FN (a macro/function name with no args) to expand and validate
# the archive. The callee sets GStreamer_ROOT_DIR and GStreamer_AUTO_DOWNLOADED.
#
# OUT parameters (all set in PARENT_SCOPE if provided):
#   CACHE_DIR_OUT  — resolved cache directory
#   ARCHIVE_OUT    — path to the downloaded primary archive (FILENAME_PRIMARY)
#   ARCHIVE2_OUT   — path to the downloaded secondary archive (FILENAME_SECONDARY, optional)
#
# This is a function so scratch variables don't leak; callers must pass OUT vars
# and read them back to propagate GStreamer_ROOT_DIR / GStreamer_AUTO_DOWNLOADED.
function(gstreamer_resolve_or_download_sdk)
    cmake_parse_arguments(ARG "" "PLATFORM;CACHE_SUBDIR;FILENAME_PRIMARY;FILENAME_SECONDARY;CACHE_DIR_OUT;ARCHIVE_OUT;ARCHIVE2_OUT" "" ${ARGN})
    if(NOT GStreamer_FIND_VERSION)
        message(FATAL_ERROR "gstreamer_resolve_or_download_sdk: GStreamer_FIND_VERSION not set")
    endif()

    foreach(_req IN ITEMS PLATFORM CACHE_SUBDIR FILENAME_PRIMARY CACHE_DIR_OUT ARCHIVE_OUT)
        if(NOT ARG_${_req})
            message(FATAL_ERROR "gstreamer_resolve_or_download_sdk: ${_req} is required")
        endif()
    endforeach()

    if(CPM_SOURCE_CACHE)
        set(_cache_dir "${CPM_SOURCE_CACHE}/${ARG_CACHE_SUBDIR}")
    else()
        set(_cache_dir "${CMAKE_BINARY_DIR}/_deps/${ARG_CACHE_SUBDIR}")
    endif()
    set(${ARG_CACHE_DIR_OUT} "${_cache_dir}" PARENT_SCOPE)

    gstreamer_download_sdk(${ARG_PLATFORM} ${GStreamer_FIND_VERSION}
        "${ARG_FILENAME_PRIMARY}" "${_cache_dir}" _primary_archive)
    set(${ARG_ARCHIVE_OUT} "${_primary_archive}" PARENT_SCOPE)

    if(ARG_FILENAME_SECONDARY AND ARG_ARCHIVE2_OUT)
        gstreamer_download_sdk(${ARG_PLATFORM}_devel ${GStreamer_FIND_VERSION}
            "${ARG_FILENAME_SECONDARY}" "${_cache_dir}" _secondary_archive)
        set(${ARG_ARCHIVE2_OUT} "${_secondary_archive}" PARENT_SCOPE)
    endif()
endfunction()
